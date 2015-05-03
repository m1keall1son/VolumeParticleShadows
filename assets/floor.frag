#version 330

in vec4 vPosition;

uniform sampler2D uOpacityTexture;

uniform mat4 uLightViewProjection;

out vec4 FragColor;

void main () {
    
    vec2 lightTextureCoordinates = vec2(uLightViewProjection * vec4(vPosition.xyz, 1.0)) * 0.5 + 0.5;
    
    float opacity = texture(uOpacityTexture, lightTextureCoordinates).a;
    
    if (lightTextureCoordinates.x < 0.0 || lightTextureCoordinates.x > 1.0 || lightTextureCoordinates.y < 0.0 || lightTextureCoordinates.y > 1.0) {
        opacity = 0.0;
    }
    
    FragColor = vec4(0.0, 0.0, 0.0, opacity * 0.5);
}