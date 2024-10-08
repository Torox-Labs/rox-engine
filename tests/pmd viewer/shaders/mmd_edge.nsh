@uniform edge_offset "edge offset"=0.02,0.02,0.02,0.0
@uniform edge_color "edge color"=0.0,0.0,0.0,1.0

@vertex
uniform vec4 edge_offset;

void main()
{
    vec4 pos=gl_Vertex;
    pos.xyz+=gl_Normal*edge_offset.xyz;
    gl_Position=gl_ModelViewProjectionMatrix*pos;
}

@fragment
uniform vec4 edge_color;
void main() { gl_FragColor=edge_color; }
