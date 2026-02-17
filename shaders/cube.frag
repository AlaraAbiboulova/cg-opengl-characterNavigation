#version 330 core
out vec4 FragColor;

struct Material {
    vec3  ambient;
    vec3  diffuse;
    vec3  specular;
    float ka;
    float kd;
    float ks;
    float shininess;
};

struct Light {
    vec3 position;
    vec3 diffuse;   // light color
};

in vec3 vWorldPos;
in vec3 vWorldN;

uniform vec3 viewPos;
uniform Material material;
uniform Light light;

// day/night factor: 1=day, 0=night
uniform float uDayT;

// Floor texture
uniform sampler2D floorTex;
uniform int   useFloorTex;
uniform float floorTexScale;

// Wall texture
uniform sampler2D wallTex;
uniform int   useWallTex;
uniform float wallTexScale;

// Wall UV mapping controls
uniform int  wallMode;     // 0 = XY (front/back), 1 = ZY (left/right)
uniform vec2 wallSizeXY;   // (ROOM_W, ROOM_H)
uniform vec2 wallSizeZY;   // (ROOM_L, ROOM_H)

// Window panel
uniform int uUseWindow;              // 1 = window panel draw call
uniform sampler2D uWindowDayTex;     // unit 1
uniform sampler2D uWindowNightTex;   // unit 2

//lamp point light
uniform int   uUseLamp;
uniform vec3  lampPos;
uniform vec3  lampColor;
uniform float lampIntensity;

// daylight window
uniform int   uUseWinLight;
uniform vec3  winLightPos;
uniform vec3  winLightDir;      
uniform vec3  winLightColor;
uniform float winLightIntensity;
uniform float winLightRange;
uniform float winInnerCos;
uniform float winOuterCos;

float saturate(float x) { return clamp(x, 0.0, 1.0); }

vec2 ComputeWallUV()
{
    vec2 uv;
    if (wallMode == 0)
        uv = (vWorldPos.xy / wallSizeXY) * wallTexScale;
    else
        uv = (vec2(vWorldPos.z, vWorldPos.y) / wallSizeZY) * wallTexScale;
    return uv;
}

float SoftCone(float theta)
{
    float eps = max(winInnerCos - winOuterCos, 0.0001);
    return saturate((theta - winOuterCos) / eps);
}

void main()
{
    // Day/Night scaling factors
    float ambientMul = mix(0.22, 0.48, uDayT);
    float lightMul   = mix(0.65, 1.05, uDayT);

    // Base colors
    vec3 baseAmbient = material.ambient;
    vec3 baseDiffuse = material.diffuse;

    // Floor
    if (useFloorTex == 1)
    {
        vec2 uv = vWorldPos.xz * floorTexScale;
        vec3 tex = texture(floorTex, uv).rgb;
        baseAmbient = tex;
        baseDiffuse = tex;
    }

    // Walls
    if (useWallTex == 1)
    {
        vec2 uv = ComputeWallUV();
        vec3 tex = texture(wallTex, uv).rgb;
        baseAmbient = tex;
        baseDiffuse = tex;
    }

    // Window panel overrides coloring
    if (uUseWindow == 1)
    {
        vec2 uv = ComputeWallUV();
        vec3 dayCol   = texture(uWindowDayTex, uv).rgb;
        vec3 nightCol = texture(uWindowNightTex, uv).rgb;

        vec3 winCol = mix(nightCol, dayCol, uDayT);
        winCol += (1.0 - uDayT) * 0.12;

        baseAmbient = winCol;
        baseDiffuse = winCol;
    }

    vec3 N = normalize(vWorldN);
    vec3 V = normalize(viewPos - vWorldPos);

    // ceiling light
    vec3 L = normalize(light.position - vWorldPos);

    vec3 ambient = material.ka * baseAmbient * ambientMul;

    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = light.diffuse * (material.kd * NdotL) * baseDiffuse * lightMul;

    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), material.shininess);
    vec3 specular = material.ks * spec * material.specular * lightMul;

    vec3 color = ambient + diffuse + specular;

    //lampa
    if (uUseLamp == 1)
    {
        vec3 LmVec = lampPos - vWorldPos;
        float lmDist = length(LmVec);
        vec3 Lm = LmVec / max(lmDist, 1e-5);

        float lmAtt = 1.0 / (1.0 + 0.08 * lmDist + 0.02 * lmDist * lmDist);

        float NdotLm = max(dot(N, Lm), 0.0);
        vec3 diffLm = lampColor * (material.kd * NdotLm) * baseDiffuse * lmAtt * lampIntensity;

        vec3 Rm = reflect(-Lm, N);
        float specLm = pow(max(dot(V, Rm), 0.0), material.shininess);
        vec3 specLmCol = material.ks * specLm * material.specular * lmAtt * lampIntensity * lampColor;

        color += diffLm + specLmCol;
    }

    // light from window
    if (uUseWinLight == 1)
    {
        vec3 Wvec = vWorldPos - winLightPos;        // window -> frag
        float wDist = length(Wvec);
        vec3 Wdir = Wvec / max(wDist, 1e-5);

        // cone (directional) + range fade + distance attenuation
        float theta = dot(normalize(winLightDir), Wdir);
        float cone = SoftCone(theta);

        float rangeFade = saturate(1.0 - (wDist / max(winLightRange, 0.001)));
        rangeFade *= rangeFade;

        float wAtt = 1.0 / (1.0 + 0.06 * wDist + 0.02 * wDist * wDist);

        vec3 Lw = normalize(-Wdir); // fragment sees light coming from window direction
        float NdotW = max(dot(N, Lw), 0.0);

        vec3 diffW = winLightColor * (material.kd * NdotW) * baseDiffuse;
        diffW *= cone * rangeFade * wAtt * winLightIntensity;

        vec3 Rw = reflect(-Lw, N);
        float specW = pow(max(dot(V, Rw), 0.0), material.shininess);
        vec3 specWCol = material.ks * specW * material.specular;
        specWCol *= cone * rangeFade * wAtt * (0.65 * winLightIntensity) * winLightColor;

        color += diffW + specWCol;
    }

    FragColor = vec4(color, 1.0);
}
