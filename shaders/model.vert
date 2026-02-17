#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 vWorldPos;
out vec3 vWorldN;
out vec2 vUV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

void main()
{
    vec4 wp = model * vec4(aPos, 1.0);
    vWorldPos = wp.xyz;

    mat3 normalMat = mat3(transpose(inverse(model)));
    vWorldN = normalMat * aNormal;

    vUV = aTexCoords;

    gl_Position = proj * view * wp;
}
