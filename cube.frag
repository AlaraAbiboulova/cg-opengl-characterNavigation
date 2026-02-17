#version 330 core
out vec4 FragColor;

//phong
struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float ka; //ambient coefficient
    float kd; //diffuse coefficient
    float ks; //specular coefficient
    float shininess;
};

struct Light {
    vec3 position;
    vec3 diffuse; //the color of the light
};

in vec3 FragPos;
in vec3 Normal;

uniform vec3 viewPos;
uniform Material material;
uniform Light light;

//Floor texture 
uniform sampler2D floorTex;
uniform int useFloorTex;      // 0 = OFF, 1 = ON
uniform float floorTexScale;  // tiling factor-how often it repeats

void main()
{
    // choose base color (material or texture)
    vec3 baseAmbient = material.ambient;
    vec3 baseDiffuse = material.diffuse;

    if (useFloorTex == 1)//override colour
    {
        vec2 uv = FragPos.xz * floorTexScale;
        vec3 texColor = texture(floorTex, uv).rgb;

        baseAmbient = texColor;
        baseDiffuse = texColor;
    }

    // ambient
    vec3 ambient = material.ka * baseAmbient;

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0); //cos to light direction
    vec3 diffuse = light.diffuse * material.kd * (diff * baseDiffuse);

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = material.ks * (spec * material.specular);

    vec3 result = ambient + diffuse + specular;
    FragColor = vec4(result, 1.0);
}
