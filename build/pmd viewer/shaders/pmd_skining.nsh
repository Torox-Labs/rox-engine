@include "skeleton.nsh"
//@include "morph.nsh"

@vertex
out vec3 pos;
out vec3 normal;
out vec2 tc;

void main()
{
    tc=gl_MultiTexCoord0.xy;

    vec3 vpos = gl_Vertex.xyz;
    //vec3 vpos=apply_morphs(gl_Vertex.xyz,gl_Vertex.w);

    pos=mix(tr(vpos,gl_MultiTexCoord1.y),tr(vpos,gl_MultiTexCoord1.x),gl_MultiTexCoord1.z);
    normal=mix(trn(gl_Normal,gl_MultiTexCoord1.y),trn(gl_Normal,gl_MultiTexCoord1.x),gl_MultiTexCoord1.z);
}

@fragment
void main() {}
