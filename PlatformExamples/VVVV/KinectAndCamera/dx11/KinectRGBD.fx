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
	
	float Alpha <float uimin=0.0; float uimax=1.0;> = 1; 
	float4 cAmb <bool color=true;String uiname="Color";> = { 1.0f,1.0f,1.0f,1.0f };
	float4x4 tTex <string uiname="Texture Transform"; bool uvspace=true; >;
	float4x4 tColor <string uiname="Color Transform";>;
};

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




