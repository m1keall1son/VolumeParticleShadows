#version 330

uniform float uParticleAlpha;

out vec4 FragColor;

void main () {
    
    float distanceFromCenter = distance(gl_PointCoord.xy, vec2(0.5, 0.5));
    
    if (distanceFromCenter > 0.5) discard;
    
    float alpha = clamp(1.0 - distanceFromCenter * 2.0, 0.0, 1.0) * uParticleAlpha;
    
    FragColor = vec4(1.,1., 1.0, alpha); //under operator requires this premultiplication
}