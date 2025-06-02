#version 330 core

in vec3 fragPos;
in vec3 normal;
in vec2 texCoord;

uniform sampler2D diffuseTexture;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 cameraPos;

uniform vec3 Ka;
uniform vec3 Ks;
uniform float q;

out vec4 FragColor;

void main()
{
    vec3 ambient = Ka * texture(diffuseTexture, texCoord).rgb;

    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * texture(diffuseTexture, texCoord).rgb;

    vec3 viewDir = normalize(cameraPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), q);
    vec3 specular = spec * Ks * lightColor;

    vec3 finalColor = ambient + diffuse + specular;
    FragColor = vec4(finalColor, 1.0);
}
