@all

varying vec2 tc;
varying vec4 linear_k;

@vertex

void main()
{
    float near=gl_ProjectionMatrix[2][3]/(gl_ProjectionMatrix[2][2]-1.0);
    float far=gl_ProjectionMatrix[2][3]/(1.0+gl_ProjectionMatrix[2][2]);

    linear_k.x=near;
    linear_k.y=(far-near)/far;

    tc=gl_MultiTexCoord0.xy;
    gl_Position=gl_Vertex;
}

@sampler depth_map "depth"

@fragment

uniform sampler2D depth_map;

void main()
{
    gl_FragColor=vec4(linear_k.x/(1.0-texture2D(depth_map,tc).r*linear_k.y));
}
