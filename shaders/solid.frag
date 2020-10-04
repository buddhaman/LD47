#version 330 core

in vec3 color;
in vec3 normal;

out vec4 FragColor;

void main()
{
    vec3 lightDir = vec3(-1, -1, -1);
    lightDir = normalize(lightDir);
    float dp = max(-dot(normal, lightDir), 0);
    float diffuse = 0.4;
    float lightFactor = diffuse * (1.0-dp) + dp;
    //lightFactor = 1.0;
    FragColor = vec4(lightFactor*color, 1.0);
}
