@include "skeleton.nsh"
@include "morph.nsh"

@vertex
out vec3 pos;
out vec3 normal;
out vec2 tc;

void main()
{
    tc=apply_morphs(gl_MultiTexCoord0.xy,gl_Vertex.w);
    vec3 vpos=apply_morphs(gl_Vertex.xyz,gl_Vertex.w);
    pos=tr(vpos,gl_MultiTexCoord1[0])*gl_MultiTexCoord2[0];
    normal=trn(gl_Normal,gl_MultiTexCoord1[0])*gl_MultiTexCoord2[0];
    for(int i=1;i<4;++i)
    {
        if(gl_MultiTexCoord2[i]>0.0)
        {
            pos+=tr(vpos,gl_MultiTexCoord1[i])*gl_MultiTexCoord2[i];
            normal+=trn(gl_Normal,gl_MultiTexCoord1[i])*gl_MultiTexCoord2[i];
        }
    }
}

@fragment
void main() {}
