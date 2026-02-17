#version 330 core
out vec4 FragColor;

in vec3 vWorldPos;
in vec3 vWorldN;
in vec2 vUV;

// ceiling light
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

// day/night factor
uniform float uDayT;

// textures
uniform sampler2D texture_diffuse1;

//override diffuse
uniform sampler2D uOverrideDiffuse;
uniform int uUseOverrideDiffuse;

uniform int uUnlit;
//albedo - base colour surface

// spotlight
uniform int   uUseSpot;
uniform vec3  spotPos;
uniform vec3  spotDir;
uniform vec3  spotColor;
uniform float spotInnerCos;
uniform float spotOuterCos;
uniform float spotIntensity;

// lamp point light
uniform int   uUseLamp;
uniform vec3  lampPos;
uniform vec3  lampColor;
uniform float lampIntensity;

// window day
uniform int   uUseWinLight;
uniform vec3  winLightPos;
uniform vec3  winLightDir;      // normalized, points into room
uniform vec3  winLightColor;
uniform float winLightIntensity;
uniform float winLightRange;
uniform float winInnerCos;
uniform float winOuterCos;

float saturate(float x){ return clamp(x, 0.0, 1.0); }

float SpotCone(vec3 fromSpotToFrag)
{
    vec3 L = normalize(fromSpotToFrag);
    float theta = dot(normalize(spotDir), L);
    float eps = max(spotInnerCos - spotOuterCos, 0.0001);
    return saturate((theta - spotOuterCos) / eps);
}

float WinCone(float theta)
{
    float eps = max(winInnerCos - winOuterCos, 0.0001);
    return saturate((theta - winOuterCos) / eps);
}

void main()
{
    vec3 N = normalize(vWorldN);

    vec3 albedo = (uUseOverrideDiffuse == 1)
        ? texture(uOverrideDiffuse, vUV).rgb
        : texture(texture_diffuse1, vUV).rgb;

    if (dot(albedo, albedo) < 1e-6)
        albedo = vec3(0.7);

    if (uUnlit == 1)
    {
        FragColor = vec4(albedo, 1.0);
        return;
    }

    vec3 V = normalize(viewPos - vWorldPos);

    // day/night multipliers (apply to ceiling light only)
    float ambientMul = mix(0.22, 0.48, uDayT);
    float lightMul   = mix(0.65, 1.05, uDayT);

    // ceiling light
    vec3 Lvec = lightPos - vWorldPos;
    float dist = length(Lvec);
    vec3 L = Lvec / max(dist, 1e-5);

    float att = 1.0 / (1.0 + 0.18 * dist + 0.06 * dist * dist);

    float NdotL = max(dot(N, L), 0.0);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(N, H), 0.0), 64.0);

    vec3 ambient  = (0.10 * ambientMul) * albedo;
    vec3 diffuse  = (NdotL * albedo) * att * lightMul * lightColor;
    vec3 specular = (0.08 * spec) * att * lightMul * lightColor;

    vec3 color = ambient + diffuse + specular;

    // spotlight
    if (uUseSpot == 1)
    {
        vec3 Svec = vWorldPos - spotPos; 
        float sDist = length(Svec);
        vec3 Sdir = Svec / max(sDist, 1e-5);

        float cone = SpotCone(Svec);
        float sAtt = 1.0 / (1.0 + 0.10 * sDist + 0.03 * sDist * sDist);

        float NdotS = max(dot(N, -Sdir), 0.0);
        vec3 sH = normalize((-Sdir) + V);
        float sSpec = pow(max(dot(N, sH), 0.0), 64.0);

        vec3 sDiffuse  = (NdotS * albedo) * cone * sAtt * spotIntensity;
        vec3 sSpecular = vec3(0.06 * sSpec) * cone * sAtt * spotIntensity;

        color += (sDiffuse + sSpecular) * spotColor;
    }

    //lamp
    if (uUseLamp == 1)
    {
        vec3 LmVec = lampPos - vWorldPos;
        float lmDist = length(LmVec);
        vec3 Lm = LmVec / max(lmDist, 1e-5);

        float lmAtt = 1.0 / (1.0 + 0.08 * lmDist + 0.02 * lmDist * lmDist);

        float NdotLm = max(dot(N, Lm), 0.0);

        vec3 Hm = normalize(Lm + V);
        float lmSpec = pow(max(dot(N, Hm), 0.0), 48.0);

        vec3 lmDiffuse  = (NdotLm * albedo) * lmAtt * lampIntensity * lampColor;
        vec3 lmSpecular = (0.05 * lmSpec)   * lmAtt * lampIntensity * lampColor;

        color += lmDiffuse + lmSpecular;
    }

    // day only from window
    if (uUseWinLight == 1)
    {
        vec3 Wvec = vWorldPos - winLightPos;  // window -> frag
        float wDist = length(Wvec);
        vec3 Wdir = Wvec / max(wDist, 1e-5);

        float theta = dot(normalize(winLightDir), Wdir);
        float cone = WinCone(theta);

        float rangeFade = saturate(1.0 - (wDist / max(winLightRange, 0.001)));
        rangeFade *= rangeFade;

        float wAtt = 1.0 / (1.0 + 0.06 * wDist + 0.02 * wDist * wDist);

        vec3 Lw = normalize(-Wdir);
        float NdotW = max(dot(N, Lw), 0.0);

        vec3 Hw = normalize(Lw + V);
        float wSpec = pow(max(dot(N, Hw), 0.0), 56.0);

        vec3 wDiffuse  = (NdotW * albedo) * cone * rangeFade * wAtt * winLightIntensity * winLightColor;
        vec3 wSpecular = (0.04 * wSpec)   * cone * rangeFade * wAtt * (0.75 * winLightIntensity) * winLightColor;

        color += wDiffuse + wSpecular;
    }

    FragColor = vec4(color, 1.0);
}
