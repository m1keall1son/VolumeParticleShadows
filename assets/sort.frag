#version 330

uniform sampler2D uDataTexture;

uniform vec2 uResolution;

uniform float uPass;
uniform float uStage;

uniform vec3 uCameraPosition;

uniform vec3 uHalfVector;

in vec2 vTexCoord0;

out vec4 FragColor;

void main () {
    
    vec2 normalizedCoordinates = vTexCoord0;//gl_FragCoord.xy / uResolution;

    vec4 self = texture(uDataTexture, vTexCoord0);
    
    float i = floor(normalizedCoordinates.x * uResolution.x) + floor(normalizedCoordinates.y * uResolution.y) * uResolution.x;
    
    float j = floor( mod( i, 2.0 * uStage ) );
    
    float compare = 0.0;
    
    if ((j < mod(uPass, uStage)) || (j > (2.0 * uStage - mod(uPass, uStage) - 1.0))) {
        compare = 0.0;
    } else {
        if (mod((j + mod(uPass, uStage)) / uPass, 2.0) < 1.0) {
            compare = 1.0;
        } else {
            compare = -1.0;
        }
    }
    
    float adr = i + compare * uPass;
    
    vec4 partner = texture(uDataTexture, vec2(floor( mod(adr, uResolution.x)) / uResolution.x, floor(adr / uResolution.x) / uResolution.y));
    
    float selfProjectedLength = dot(uHalfVector, self.xyz);
    float partnerProjectedLength = dot(uHalfVector, partner.xyz);
    
    FragColor = (selfProjectedLength * compare < partnerProjectedLength * compare) ? self : partner;
}