@sampler base_map "diffuse"

@all
varying vec2 tc;

@vertex

void main()
{
    tc=gl_MultiTexCoord0.xy;
    gl_Position=gl_ModelViewProjectionMatrix*gl_Vertex;
}

@fragment
uniform sampler2D base_map;

void main()
{
    gl_FragColor=texture2D(base_map,tc);
}
