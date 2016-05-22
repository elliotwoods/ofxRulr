//@author: elliotwoods
//@help: apply color camera onto kinect point cloud
//@tags: color
//@credits: 

Texture2D colorTexture <string uiname="Color Texture";>;
Texture2D worldTexture <string uiname="World Texture";>;

SamplerState g_samNearest <string uiname="Sampler State";>
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
};

SamplerState g_samLinear <string uiname="Sampler State";>
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;
};

cbuffer cbPerDraw : register( b0 )
{
	float4x4 tVP : VIEWPROJECTION;
};


cbuffer cbPerObj : register( b1 )
{
	float4x4 tW : WORLD;
	float4x4 tKinectSetup <string uiname="Kinect World Setup Transform";>;
	float4x4 tCamera <string uiname="Camera ViewProjection";>;
	float4 Distortion;
	
	float Alpha <float uimin=0.0; float uimax=1.0;> = 1; 
	float4 cAmb <bool color=true;String uiname="Color";> = { 1.0f,1.0f,1.0f,1.0f };
	float4x4 tTex <string uiname="Texture Transform"; bool uvspace=true; >;
	float4x4 tColor <string uiname="Color Transform";>;
};

//these undistort functions are adapted from Undistort.fxh
float2 tranf(float2 TexCd, float2 PrincipalPoint, float2 FocalLength, float2 Resolution)
{
  return (TexCd * FocalLength + PrincipalPoint) / Resolution;
}

float2 tranf_inv(float2 TexCd, float2 PrincipalPoint, float2 FocalLength, float2 Resolution)
{
  return (TexCd * Resolution - PrincipalPoint) / FocalLength;
}

float2 Distort(float2 p, float k1, float k2, float p1, float p2)
{

    float sq_r = p.x*p.x + p.y*p.y;

    float2 q = p;
    float a = 1 + sq_r * (k1 + k2 * sq_r);
    float b = 2*p.x*p.y;

    q.x = a*p.x + b*p1 + p2*(sq_r+2*p.x*p.x);
    q.y = a*p.y + p1*(sq_r+2*p.y*p.y) + b*p2;

    return q;
}

float2 Undistort(float2 TexCd, float2 FocalLength, float2 PrincipalPoint, float4 Distortion, float2 Resolution)
{
	float2 t = tranf_inv(TexCd, PrincipalPoint, FocalLength, Resolution);
	t = Distort(t, Distortion[0], Distortion[1], Distortion[2], Distortion[3]);
	t = tranf(t, PrincipalPoint, FocalLength, Resolution);

	return t;
}

struct VS_IN
{
	float4 PosO : POSITION;
	float4 TexCd : TEXCOORD0;
};

struct vs2ps
{
    float4 PosWVP: SV_POSITION;
    float4 TexCd: TEXCOORD0;
};

vs2ps VS(VS_IN input)
{
	vs2ps Out = (vs2ps)0;
	float worldTextureWidth, worldTextureHeight, worldTextureLevels;
	worldTexture.GetDimensions(0, worldTextureWidth, worldTextureHeight, worldTextureLevels);
	
	//find world position
	float4 PosW;
	{
		float4 kinectObjectPosition = worldTexture.SampleLevel(g_samNearest, input.TexCd.xy, 0);
		kinectObjectPosition.w = kinectObjectPosition.z > 0.1f;
		float4 PosO = mul(kinectObjectPosition, tKinectSetup);
		PosW = mul(PosO, tW);
	    Out.PosWVP  = mul(PosW,tVP);
	}
	
	//project texture coords
	{
		float4 positionInColorCamera = mul(PosW, tCamera);
		positionInColorCamera /= positionInColorCamera.w;
		positionInColorCamera.xy = Distort(positionInColorCamera.xy / 2.0f, Distortion[0], Distortion[1], Distortion[2], Distortion[3]) * 2.0f;
		Out.TexCd.x = (1.0f + positionInColorCamera.x) / 2.0f;
		Out.TexCd.y = (1.0f - positionInColorCamera.y) / 2.0f;
		
		Out.TexCd.zw = float2(0, 1);
	}
	
    return Out;
}




float4 PS(vs2ps In): SV_Target
{
    float4 col = colorTexture.Sample(g_samLinear,In.TexCd.xy) * cAmb;
	if(any(abs(In.TexCd.xy - 0.5f) > 0.5f)) {
		discard;
	}
	
	col = mul(col, tColor);
	col.a *= Alpha;
    return col;
}





technique10 Constant
{
	pass P0
	{
		SetVertexShader( CompileShader( vs_4_0, VS() ) );
		SetPixelShader( CompileShader( ps_4_0, PS() ) );
	}
}




