#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 vWorldPos;
out vec3 vWorldN;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform mat3 normalMat;

void main()
{
    vec4 wp = model * vec4(aPos, 1.0);
    vWorldPos = wp.xyz;
    vWorldN   = normalize(normalMat * aNormal);

    gl_Position = proj * view * wp;
}
