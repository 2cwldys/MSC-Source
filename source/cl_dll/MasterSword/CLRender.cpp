// Triangle rendering, if any

#include "inc_weapondefs.h"
#include "CLRender.h"
#include "CLGlobal.h"

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "HUDScript.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI

#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "const.h"
#include "com_model.h"
#include "studio.h"
#include "entity_state.h"
#include "studio_util.h"
#include "r_studioint.h"
#include "ref_params.h"

//#include "D3dx8tex.h"

#include <windows.h>							// Header File For Windows
//#include <gl/glext.h>

//Thothie TWHL project JUN2010_22 - http://twhl.co.za/articulator.php?art=170&form=old
#include <gl/gl.h>
//include <gl/glext.h> //AUG2013_21 - Nvidia update - no longer need this header
#include <cg/cg.h>
#include <cg/cgGL.h>
#pragma comment( lib, "cg.lib" )
#pragma comment( lib, "cgGL.lib" )

//originals from MSC
//#include <gl\gl.h>								// Header File For The OpenGL32 Library
#include <gl\glu.h>								// Header File For The GLu32 Library
#include <gl\glaux.h>							// Header File For The GLaux Library
#include "logfile.h"

//OGL
void DeleteGLTextures( );
PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB = NULL;
PFNGLACTIVETEXTUREARBPROC glActiveTextureARB = NULL;


//Environment Manager
float CEnvMgr::m_LightGamma = 2.5;
float CEnvMgr::m_MaxViewDistance = 4096;
CEnvMgr::fog_t CEnvMgr::m_Fog = { false, Vector(0,0,0), 0.01, 0.01, 10000, GL_EXP };


//Half-life callback
#define DLLEXPORT __declspec( dllexport )
extern "C"
{
	void DLLEXPORT HUD_DrawNormalTriangles( void );
	void DLLEXPORT HUD_DrawTransparentTriangles( void );
};

//TWHL Project - Thothie JUN2010_22
#include "r_studioint.h"

extern engine_studio_api_t IEngineStudio;

bool g_bInitialised = false;

//PFNGLACTIVETEXTUREARBPROC glActiveTextureARB = NULL;
//PFNGLMULTITEXCOORD2FARBPROC glMultiTexCoord2fARB = NULL;

CGcontext g_cgContext;
CGprofile g_cgVertProfile;
CGprofile g_cgFragProfile;

CGprogram g_cgVP_GlowDarken;
CGprogram g_cgFP_GlowDarken;

CGprogram g_cgVP_GlowBlur;
CGprogram g_cgFP_GlowBlur;

CGprogram g_cgVP_GlowCombine;
CGprogram g_cgFP_GlowCombine;

CGparameter g_cgpVP0_ModelViewMatrix;
CGparameter g_cgpVP1_ModelViewMatrix;
CGparameter g_cgpVP1_XOffset;
CGparameter g_cgpVP1_YOffset;
CGparameter g_cgpVP2_ModelViewMatrix;

unsigned int g_uiSceneTex;
unsigned int g_uiBlurTex;
//[/TWHL]

//extern Vector g_ViewForward, g_ViewRight, g_ViewUp;
extern float v_ViewDist;
void VectorAngles( const float *forward, float *angles );
void VGUIImages_NewLevel( );


CParticle::CParticle( ) 
{ 
	m_Width = 0; m_Origin = g_vecZero; m_Texture = NULL; 
	m_Color = Color4F( 1.0f, 1.0f, 1.0f, 1.0f );
	m_Brightness = 1.0f;
	m_DirForward = g_vecZero;
	m_DirRight = g_vecZero;
	m_DirUp = g_vecZero;
	m_DoubleSided = true;
	m_ContinuedParticle = false;
	m_Square = true;
	m_GLTex = 0;
	m_RenderMode = kRenderNormal;
	m_SpriteFrame = 0;

	m_TexCoords[0] = Vector2D( 1, 0 );
	m_TexCoords[1] = Vector2D( 1, 1 );
	m_TexCoords[2] = Vector2D( 0, 1 );
	m_TexCoords[3] = Vector2D( 0, 0 );
}

void CParticle::SetAngles( Vector Angles )
{
	EngineFunc::MakeVectors( Angles, m_DirForward, m_DirRight, m_DirUp );
}

void CParticle::BillBoard( )
{
	//Make this face the player
	m_DirForward = ViewMgr.Params->forward;
	m_DirRight = ViewMgr.Params->right;
	m_DirUp = ViewMgr.Params->up;
}
bool CParticle::LoadTexture( msstring_ref Name )
{
	HSPRITE Sprite = MSBitmap::GetSprite( Name );
	if( !Sprite ) return false;

	m_Texture = (model_s *)gEngfuncs.GetSpritePointer( Sprite );
	if( !m_Texture ) return false;

	return true;
}
void CParticle::Render( ) 
{
	//return;
	Vector &vForward = m_DirForward, &vRight = m_DirRight, &vUp = m_DirUp;

	Vector Points[4];

	float HalfWidth = m_Width/2.0f;
	float HalfHeight = m_Square ? HalfWidth : m_Height/2.0f;
	Vector SideRay = vRight * HalfWidth;
	Vector VerticalRay = vUp * HalfHeight;

	Points[0] = m_Origin - SideRay - VerticalRay;
	Points[1] = m_Origin + SideRay - VerticalRay;
	Points[2] = m_Origin + SideRay + VerticalRay;
	Points[3] = m_Origin - SideRay + VerticalRay;
	

	// Create me a triangle

	//gEngfuncs.pTriAPI->RenderMode( kRenderNormal );

	if( m_GLTex ) glBindTexture( GL_TEXTURE_2D, m_GLTex );
	else if( m_Texture ) gEngfuncs.pTriAPI->SpriteTexture( m_Texture, m_SpriteFrame );
	else 
		glBindTexture( GL_TEXTURE_2D, 0 );

	gEngfuncs.pTriAPI->RenderMode( m_RenderMode );
	gEngfuncs.pTriAPI->CullFace( m_DoubleSided ? TRI_NONE : TRI_FRONT );
	gEngfuncs.pTriAPI->Brightness( m_Brightness );

	gEngfuncs.pTriAPI->Color4f( m_Color.r, m_Color.g, m_Color.b, m_Color.a );

	//glDisable( GL_DEPTH_TEST );
	//glEnable( GL_BLEND );
	/*glBegin( GL_QUADS );
	glColor4fv( (float *)&m_Color.r );
	
	glTexCoord2f( m_TexCoords[0].x, m_TexCoords[0].y ); glVertex3fv( Points[0] );
	glTexCoord2f( m_TexCoords[1].x, m_TexCoords[1].y ); glVertex3fv( Points[3] );
	glTexCoord2f( m_TexCoords[2].x, m_TexCoords[2].y ); glVertex3fv( Points[2] );
	glTexCoord2f( m_TexCoords[3].x, m_TexCoords[3].y ); glVertex3fv( Points[1] );
	glEnd( );*/

	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
 	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
 	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	if( !m_ContinuedParticle )
		gEngfuncs.pTriAPI->Begin( TRI_QUADS );

	gEngfuncs.pTriAPI->TexCoord2f( m_TexCoords[0].x, m_TexCoords[0].y );
	gEngfuncs.pTriAPI->Vertex3fv( Points[0] );

	gEngfuncs.pTriAPI->TexCoord2f( m_TexCoords[1].x, m_TexCoords[1].y );
	gEngfuncs.pTriAPI->Vertex3fv( Points[3] );

	gEngfuncs.pTriAPI->TexCoord2f( m_TexCoords[2].x, m_TexCoords[2].y );
	gEngfuncs.pTriAPI->Vertex3fv( Points[2] );

	gEngfuncs.pTriAPI->TexCoord2f( m_TexCoords[3].x, m_TexCoords[3].y );
	gEngfuncs.pTriAPI->Vertex3fv( Points[1] );

	if( !m_ContinuedParticle )
		gEngfuncs.pTriAPI->End();

	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
}



struct skyface_t
{
	CParticle Face;

	//Vector Offset;
};

#define SKYFILENAME_PREFIX "gfx/env/"
struct
{
	Vector Dir;	//Dir the wall is facing.  It gets scooted back in the opp. direction
	msstring_ref FileNameSuffix;
	bool Rotate;
} g_SkyBoxInfo[6] =
{
	{ Vector(  0,  180,  0), "ft", false },
	{ Vector(  0,   90,  0), "lf", false },
	{ Vector(  0,    0,  0), "bk", false },
	{ Vector(  0,  270,  0), "rt", false },
	{ Vector(-90,    0,  0), "dn", false },
	{ Vector( 90,    0,  0), "up", true },
};

class CSkyBox
{
public:
	msstring m_SkyName;
	mslist<skyface_t> Faces;
	int m_uiNextTexIdx;

	CSkyBox( )
	{
		m_uiNextTexIdx = 1;
		m_SkyName = "g_morning";

		foreach( i, 6 )
		{
			Faces.add( skyface_t() );

			CParticle &Face = Faces[i].Face;
			Face.m_Color = Color4F( 1, 1, 1, 1 );
			Face.m_Brightness = 1.0f;
		}
	}

	//Called once each level load
	void Setup( )	//Be sure to call after EngineFunc stuff is valid
	{
		foreach( i, Faces.size() )
		{
			CParticle &Face = Faces[i].Face;
			Face.SetAngles( g_SkyBoxInfo[i].Dir );
			if( g_SkyBoxInfo[i].Rotate )
			{
				Vector2D Tmp = Face.m_TexCoords[0];
				Face.m_TexCoords[0] = Vector2D( 0, 0 );
				Face.m_TexCoords[1] = Vector2D( 1, 0 );
				Face.m_TexCoords[2] = Vector2D( 1, 1 );
				Face.m_TexCoords[3] = Vector2D( 0, 1 );
			}
		}

		ChangeTexture( m_SkyName );
	}

	//Dynamic.  Can change textures anytime
	void ChangeTexture( msstring_ref NewTexture )
	{
		m_SkyName = NewTexture;
		foreach( i, Faces.size() )
		{
			msstring FileName = /*msstring(EngineFunc::GetGameDir()) + "/" +*/ msstring(SKYFILENAME_PREFIX) + NewTexture + g_SkyBoxInfo[i].FileNameSuffix + ".tga";
			Faces[i].Face.m_GLTex = 0;
			LoadGLTexture( FileName, Faces[i].Face.m_GLTex );
		}
	}

	void Render( )
	{
		//if( MSGlobals::GameScript )
		//	MSGlobals::GameScript->CallScriptEvent( "game_render_sky" );

		foreach( i, Faces.size() )
		{
			CParticle &Face = Faces[i].Face;
			Face.m_Width = v_ViewDist - 1;

			Face.m_Origin = ViewMgr.Origin - Face.m_DirForward * Face.m_Width / 2.0f;
			Face.m_ContinuedParticle = false;
			Face.Render( );
		}
	}
};

CSkyBox g_CustomSkyBox;
CParticle g_Tint;

void CEnvMgr::Init( )
{
	m_MaxViewDistance = EngineFunc::CVAR_GetFloat( "sv_zmax" );
	g_CustomSkyBox.Setup( );
	g_Tint.m_Color = Color4F( 0, 0, 0, 0 );
}
void CEnvMgr::InitNewLevel( )
{
	CMirrorMgr::InitMirrors( );
	VGUIImages_NewLevel( );
	logfile << "[InitNewLevel Complete]" << "\r\n";
}
void CEnvMgr::ChangeSkyTexture( msstring_ref NewTexture )
{
	g_CustomSkyBox.ChangeTexture( NewTexture );
	g_CustomSkyBox.Setup(); //Thothie DEC2014_02 - trying to make setenv sky.texture work.
	g_CustomSkyBox.Render();  //Thothie DEC2014_02 - trying to make setenv sky.texture work.
}

void CEnvMgr::RenderSky( )
{
	g_CustomSkyBox.Render( );
}


void Surface_ResetLighting( msurface_t *pSurface )
{
	pSurface->cached_dlight = 1;
}

//Traverses all nodes and leafs, calling Func on each surface found
void TraverseAllNodes( mnode_t *pNode, void *Func )
{
	if( !pNode )
		return;

	mnode_t &Node = *pNode;
	
	if( Node.contents == CONTENTS_SOLID )
		return;

	if( Node.contents < 0 )
	{
		//Call Function on Leaf
		mleaf_t &Leaf = *(mleaf_t *)&Node;
		foreach( s, Leaf.nummarksurfaces )
			(*(ParseAllSurfacesFunc *)Func)( Leaf.firstmarksurface[s] );
		return;
	}

	foreach( i, 2 )
		TraverseAllNodes( Node.children[i], Func );
}

void CEnvMgr::SetLightGamma( float Value )
{
	m_LightGamma = Value;
	EngineFunc::CVAR_SetFloat( "lightgamma", m_LightGamma );
		
	TraverseAllNodes( gEngfuncs.GetEntityByIndex( 0 )->model->nodes, Surface_ResetLighting );
}


void CEnvMgr::ChangeTint( Color4F &Color )
{
	g_Tint.m_ContinuedParticle = false;
	g_Tint.m_DoubleSided = false;
	g_Tint.m_Color = Color;
	g_Tint.m_Brightness = 0.0f;
	g_Tint.m_Width = 500;
	g_Tint.m_RenderMode = kRenderTransAlpha;
}

//Draw transparent stuff
void CEnvMgr::Think_DrawTransparentTriangles( )
{
	bool HideTint = CMirrorMgr::m_CurrentMirror.Enabled		//Hide while rendering in mirror
					|| MSCLGlobals::CharPanelActive ||		//Hide while choosing character
					!g_Tint.m_Color.a;						//Hide if alpha is zero

	if( !HideTint )
	{
		g_Tint.m_Origin = ViewMgr.Params->vieworg + ViewMgr.Params->forward * 4.2;
		g_Tint.BillBoard();
		g_Tint.Render( );
	}
}
void CEnvMgr::Cleanup( )
{
	//Unload all OGL Textures (skybox)
	DeleteGLTextures( ); //Thothie JAN2011_03, restore texture cleanup
}

/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/

void ModifyLevel( );
void RenderFog( );
bool FindSkyHeight( Vector Origin, float &SkyHeight );
bool UnderSky( Vector Origin ); //Thothie AUG2010_03
int OldVisFrame = -1;
int OldContents = CONTENTS_EMPTY;
//void Mirror_MirrorVisibleSurfaces( );
//void Mirror_UnMirrorVisibleSurfaces( );

int CRender::m_OldHLTexture[10] = { 0 };
bool CRender::m_OldMultiTextureEnabled = false;
float CRender::m_RT_SizeRatio = 1.0f;
uint CRender::m_RT_Width = 256;
uint CRender::m_RT_Height = 256;
float CRender::m_RT_TexU = 1.0f;
float CRender::m_RT_TexV = 1.0f;

#define MS_GL_ATTRIBUTES  GL_ALL_ATTRIB_BITS
//#define MS_GL_ATTRIBUTES	

void CRender::PushHLStates( )
{
	if( !glActiveTextureARB )
		return;

	glPushAttrib( MS_GL_ATTRIBUTES );

	glActiveTextureARB( GL_TEXTURE0_ARB );
	glGetIntegerv( 0x8069, &m_OldHLTexture[0] );		//GL_TEXTURE_2D_BINDING = 0x8069

	//Workaround - 
	//Half-life sometimes uses Texture1 to draw normal textures and other times Texture2
	//It just depends on where you're standing.  Here I check whether HL is using Texture2,
	//disable it, use texture1, then set it back to Texture2 when done
	glActiveTextureARB( GL_TEXTURE1_ARB );
	glGetIntegerv( 0x8069, &m_OldHLTexture[1] );		//GL_TEXTURE_2D_BINDING = 0x8069

	m_OldMultiTextureEnabled = glIsEnabled( GL_TEXTURE_2D ) ? true : false;
	glDisable( GL_TEXTURE_2D );

	glActiveTextureARB( GL_TEXTURE0_ARB );
	glEnable( GL_TEXTURE_2D );

	/*glEnable( GL_COLOR_MATERIAL );
 	glColorMaterial( GL_FRONT, GL_AMBIENT_AND_DIFFUSE );
	glColorMaterial( GL_FRONT, GL_EMISSION );
	glColorMaterial( GL_FRONT, GL_SPECULAR );*/

	//---------------

	/*glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);*/
}
void CRender::PopHLStates( )
{
	if( !glActiveTextureARB )
		return;

	//Part of the Texture1/Texture2 Workaround
	//Set the active texture back to what HL was using before
	//----------------------------------------
	glPopAttrib( );

	glActiveTextureARB( GL_TEXTURE1_ARB );
	glBindTexture( GL_TEXTURE_2D, m_OldHLTexture[1] );
	if( m_OldMultiTextureEnabled ) glEnable( GL_TEXTURE_2D );
	else glDisable( GL_TEXTURE_2D );

	glActiveTextureARB( GL_TEXTURE0_ARB );
	glBindTexture( GL_TEXTURE_2D, m_OldHLTexture[0] );

	if( m_OldMultiTextureEnabled ) 
		glActiveTextureARB( GL_TEXTURE1_ARB );

	//static float color[4] = { 0, 0, 0, 0 };
	//int Param = GL_MODULATE;
	//glTexEnviv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, &Param );
	//glTexEnvfv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, color );

	//----------------------------------------
}



void DLLEXPORT HUD_DrawNormalTriangles( void )
{
	DBG_INPUT;
	startdbg;

	dbg( "Begin" );

	/*if( CMirrorMgr::m_CurrentMirror.Enabled 
		&& OldVisFrame > -1 )
	{
		cl_entity_t *clWorldEnt = gEngfuncs.GetEntityByIndex( 0 );
		if( clWorldEnt->model->nodes[0].visframe != 0 )
		{
			//The world was marked to be hidden for this mirror, but was drawn
			//The player changed leafs, and thus the world visframe was updated and the world drawn

			//The mirror will attempt to render *again* this frame. 
			//This is done by adding a copy of the mirror to the end of the RdrMirrors list.  After the copy
			//is done rendering, it is deleted
			CMirror MirrorCopy;
			CMirrorMgr::m_RdrMirrors.add( MirrorCopy );
			CMirrorMgr::m_CurrentMirror.Mirror = &CMirrorMgr::m_RdrMirrors[CMirrorMgr::m_CurrentMirror.Index];	//The pointer changed, because I re-allocated
			MirrorCopy = *CMirrorMgr::m_CurrentMirror.Mirror;
			MirrorCopy.Frame_NoRender = false;
			MirrorCopy.Frame_IsCopy = true;

			//Don't render the current mirror this frame, since the world was drawn on top of it
			CMirrorMgr::m_CurrentMirror.Mirror->Frame_NoRender = true;
		}
	}*/


	enddbg;
}


/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/


//Draw Mirrors
void DLLEXPORT HUD_DrawTransparentTriangles( void )
{
	DBG_INPUT;

	startdbg;
	dbg( "Begin" );

	CRender::PushHLStates( );


	RenderFog( );
	CMirrorMgr::HUD_DrawTransparentTriangles( );
	CEnvMgr::Think_DrawTransparentTriangles( );
	gHUD.m_HUDScript->Effects_DrawTransPararentTriangles( );


	CRender::PopHLStates( );

	/*if( OldVisFrame > -1 )
	{
		cl_entity_t *clWorldEnt = gEngfuncs.GetEntityByIndex( 0 );
		if( clWorldEnt->model->nodes[0].visframe == 0 )
			clWorldEnt->model->nodes[0].visframe = OldVisFrame;
	}*/

	enddbg;
}


void CRender::Cleanup( )
{
	CleanupWGL( );
	CEnvMgr::Cleanup( );
	CMirrorMgr::Cleanup( );
}


void RenderFog( )
{
	if( CEnvMgr::m_Fog.Enabled )
	{
		glEnable( GL_FOG );
		glFogfv( GL_FOG_COLOR, CEnvMgr::m_Fog.Color );
 		glFogi( GL_FOG_MODE, CEnvMgr::m_Fog.Type );		//GL_EXP, GL_LINEAR
		glFogf( GL_FOG_DENSITY, CEnvMgr::m_Fog.Density );
 		glFogf( GL_FOG_START, CEnvMgr::m_Fog.Start );
 		glFogf( GL_FOG_END, CEnvMgr::m_Fog.End );
	}
 	else
	{
		glDisable( GL_FOG );
	}
}


//VGUI_Image3D - 3D HUD Sprite
//============
#include "vgui_TeamFortressViewport.h"
#include "MasterSword/vgui_MSControls.h"

mslist<VGUI_Image3D *> g_VGUIImages;
void VGUIImages_NewLevel( )
{
	//Reload the TGA textures for the 3D VGUI Images
	foreach( i, g_VGUIImages.size() )
	{
		if( g_VGUIImages[i]->m_TGAorSprite )
		{
			g_VGUIImages[i]->m_ImageLoaded = false;
			g_VGUIImages[i]->LoadImg( );
		}
	}
	logfile << "[VGUIImages_NewLevel Complete]" << "\r\n";
}

VGUI_Image3D::VGUI_Image3D( const char *pszImageName, bool TGAorSprite, bool Delayed, int x, int y, int wide, int tall )
	: CImageDelayed( pszImageName, TGAorSprite, Delayed, x, y, wide, tall )
{
	init( );
}
void VGUI_Image3D::init( )
{
	m_Particle = new CParticle();
	m_Particle->m_ContinuedParticle = false;
	m_Particle->m_DoubleSided = true;
	m_Particle->m_Color = Color4F( 1, 1, 1, 1 );
	m_Particle->m_Brightness = 1.0f;
	m_Particle->m_Square = false;
	m_Particle->m_RenderMode = kRenderTransAlpha;
	g_VGUIImages.add( this );
}


void VGUI_Image3D::LoadImg( )
{
	startdbg;
	if( m_ImageLoaded )
		return;

	if( !m_Particle )
		return;

	if( m_TGAorSprite )
	{
		dbg( "Load TGA" );
		msstring FileName = msstring("gfx/vgui/") + m_ImageName + ".tga";

		dbg( msstring("TGA Name = ") + FileName );

		loadtex_t LoadTex;
		dbg( "Call LoadGLTexture" );
		m_ImageLoaded = LoadGLTexture( FileName, LoadTex );
		if( m_ImageLoaded )
		{
			dbg( "LoadGLTexture Succeeded" );
			m_Particle->m_GLTex = LoadTex.GLTexureID;
			m_Particle->m_TexCoords[0] = Vector2D( LoadTex.CoordU, 0 );
			m_Particle->m_TexCoords[1] = Vector2D( LoadTex.CoordU, LoadTex.CoordV );
			m_Particle->m_TexCoords[2] = Vector2D( 0, LoadTex.CoordV );
			m_Particle->m_TexCoords[3] = Vector2D( 0, 0 );
		}
		dbg( "Post Call LoadGLTexture" );
		
	}
	else
	{
		CImageDelayed::LoadImg( );
		m_Particle->m_Texture = (model_s *)gEngfuncs.GetSpritePointer( m_SpriteHandle );
		m_ImageLoaded = m_Particle->m_Texture ? true : false;
	}

	enddbg;
}
void VGUI_Image3D::paintBackground( )
{
	if( !m_ImageLoaded )
		return;

	if( !m_Particle )
		return;

	CRender::PushHLStates( );

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	//m_Particle->BillBoard();
	m_Particle->m_Width = getWide();
	m_Particle->m_Height = getTall();
	m_Particle->m_SpriteFrame = m_Frame;
	m_Particle->m_DirForward = Vector( 0, 0, 1 );
	float VerticalMultiplier = m_TGAorSprite ? -1 : 1;
	m_Particle->m_DirRight = Vector( -1, 0, 0 );
	m_Particle->m_DirUp = Vector( 0, 1 * VerticalMultiplier, 0 );		//m_DirUp is actually pointing down here
	m_Particle->m_Origin = (-m_Particle->m_DirRight * m_Particle->m_Width / 2.0f) + (m_Particle->m_DirUp * VerticalMultiplier * m_Particle->m_Height / 2.0f);
	m_Particle->Render( );

	CRender::PopHLStates( );
}

VGUI_Image3D::~VGUI_Image3D( )
{
	foreach( i, g_VGUIImages.size() )
		if( g_VGUIImages[i] == this )
			{ g_VGUIImages.erase( i ); break; }
}


//MS OGL extention stuff

struct gltexture_t : loadtex_t
{
	msstring Name;
};
mslist<gltexture_t> g_TextureList;

bool LoadGLTexture(  const char *FileName, loadtex_t &LoadTex )
{
	bool Loaded = false;

	startdbg;

	dbg( "Enter LoadGLTexture()" );
	dbg( "Find Existing texture..." );
	foreach( i, g_TextureList.size() )
	{
		if( g_TextureList[i].Name == FileName )
		{
			LoadTex = (loadtex_t)g_TextureList[i];
			dbg( "Existing texture found" );
			return true;
		}
	}

	dbg( "Existing texture not found, Call LoadTextureFile" );
	Loaded = Tartan::LoadTextureFile( FileName, LoadTex );

	if( Loaded )
	{
		dbg( "Texture Found" );
		gltexture_t Newtexture;
		loadtex_t &LT = Newtexture;
		LT = LoadTex;
		Newtexture.Name = FileName;
		g_TextureList.add( Newtexture );
	}
	else
	{
		dbg( "Texture not found" );
		Print( "Missing MS Texture: %s\n", FileName );
	}

	enddbg;

	return Loaded;
}

bool LoadGLTexture(  const char *FileName, uint &TextureID )
{
	loadtex_t LoadTex;
	bool Success = LoadGLTexture( FileName, LoadTex );
	if( Success ) TextureID = LoadTex.GLTexureID;

	return Success;
}

void DeleteGLTextures( )
{
	foreach( i, g_TextureList.size() )
		glDeleteTextures( 1, &g_TextureList[i].GLTexureID );
	g_TextureList.clear( );
}

/*void DeleteGLTexure( int &TextureID )
{
	glDeleteTextures( 1, &TextureID );
}*/

void GetCompatibleTextureSize( uint SizeW, uint SizeH, uint &outNewSizeW, uint &outNewSizeH, float &outTexCoordU, float &outTexCoordV )
{
	//Force the texture to have dimensions 2^X
	float LargestDimension = SizeW > SizeH ? SizeW : SizeH;
	float Power = logf( LargestDimension ) / logf( 2 );
	int IntPower = (int)Power;
	if( Power > IntPower ) IntPower++;	//Dimension is in-between standardized texure sizes.  Use the next highest size
	float TexSize = pow(2,(int)IntPower);

	//Cap texure size at GL_MAX_TEXTURE_SIZE
	int TexSizeMax = 0;
	glGetIntegerv( GL_MAX_TEXTURE_SIZE, &TexSizeMax );
	if( TexSize > TexSizeMax ) TexSize = (float)TexSizeMax;

	outNewSizeW = outNewSizeH = TexSize;
	outTexCoordU = SizeW / (float)outNewSizeW;
	outTexCoordV = SizeH / (float)outNewSizeH;
}

//Thothie TWHL project JUN2010_22
bool LoadProgram(CGprogram* pDest, CGprofile profile, const char* szFile)
{
     const char* szGameDir = gEngfuncs.pfnGetGameDirectory();
     char file[512];
     sprintf(file, "%s/%s", szGameDir, szFile);

     *pDest = cgCreateProgramFromFile(g_cgContext, CG_SOURCE, file, profile, "main", 0);
     if (!(*pDest)) {
          MessageBox(NULL, cgGetErrorString(cgGetError()), NULL, NULL);
          return false;
     }
     
     cgGLLoadProgram(*pDest);
	
     return true;
}

void InitScreenGlow(void)
{
     if (IEngineStudio.IsHardware() != 1)
          return;

     //gEngfuncs.pfnRegisterVariable("ms_bloom_level", "4", 0);
	 
     // OPENGL EXTENSION LOADING

     glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC)wglGetProcAddress("glActiveTextureARB"); 
 

     // TEXTURE CREATION

     unsigned char* pBlankTex = new unsigned char[ScreenWidth*ScreenHeight*3];
     memset(pBlankTex, 0, ScreenWidth*ScreenHeight*3);

     glGenTextures(1, &g_uiSceneTex);
     glBindTexture(GL_TEXTURE_RECTANGLE_NV, g_uiSceneTex);
     glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB8, ScreenWidth, ScreenHeight, 0, GL_RGB8, GL_UNSIGNED_BYTE, pBlankTex);

     glGenTextures(1, &g_uiBlurTex);
     glBindTexture(GL_TEXTURE_RECTANGLE_NV, g_uiBlurTex);
     glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB8, ScreenWidth/2, ScreenHeight/2, 0, GL_RGB8, GL_UNSIGNED_BYTE, pBlankTex);

     delete[] pBlankTex;

     g_bInitialised = true; 
 

     // CG INITIALISATION

     g_cgContext = cgCreateContext();
     if (!g_cgContext) {
          MessageBox(NULL, "Couldn't make Cg context", NULL, NULL);
          return;
     } 
 

     // VERTEX PROFILE

     g_cgVertProfile = cgGLGetLatestProfile(CG_GL_VERTEX);
     if (g_cgVertProfile == CG_PROFILE_UNKNOWN) {
          MessageBox(NULL, "Couldn't fetch valid VP profile", NULL, NULL);
          return;
     }

     cgGLSetOptimalOptions(g_cgVertProfile); 
 

     // VP LOADING

     if (!LoadProgram(&g_cgVP_GlowDarken, g_cgVertProfile, "cgprograms/glow_darken_vp.cg"))
          return;

     if (!LoadProgram(&g_cgVP_GlowBlur, g_cgVertProfile, "cgprograms/glow_blur_vp.cg"))
          return;

     if (!LoadProgram(&g_cgVP_GlowCombine, g_cgVertProfile, "cgprograms/glow_combine_vp.cg"))
          return; 
 

     // VP PARAM GRABBING

     g_cgpVP0_ModelViewMatrix = cgGetNamedParameter(g_cgVP_GlowDarken, "ModelViewProj");

     g_cgpVP1_ModelViewMatrix = cgGetNamedParameter(g_cgVP_GlowBlur, "ModelViewProj");
     g_cgpVP1_XOffset = cgGetNamedParameter(g_cgVP_GlowBlur, "XOffset");
     g_cgpVP1_YOffset = cgGetNamedParameter(g_cgVP_GlowBlur, "YOffset");

     g_cgpVP2_ModelViewMatrix = cgGetNamedParameter(g_cgVP_GlowCombine, "ModelViewProj"); 
 


     // FRAGMENT PROFILE

     g_cgFragProfile = cgGLGetLatestProfile(CG_GL_FRAGMENT);
     if (g_cgFragProfile == CG_PROFILE_UNKNOWN) {
          MessageBox(NULL, "Couldn't fetch valid FP profile", NULL, NULL);
          return;
     }

     cgGLSetOptimalOptions(g_cgFragProfile); 
 


     // FP LOADING

     if (!LoadProgram(&g_cgFP_GlowDarken, g_cgFragProfile, "cgprograms/glow_darken_fp.cg"))
          return;

     if (!LoadProgram(&g_cgFP_GlowBlur, g_cgFragProfile, "cgprograms/glow_blur_fp.cg"))
          return;

     if (!LoadProgram(&g_cgFP_GlowCombine, g_cgFragProfile, "cgprograms/glow_combine_fp.cg"))
          return;
} 

void DrawQuad(int width, int height)
{
     glBegin(GL_QUADS);

     glTexCoord2f(0,0);
     glVertex3f(0, 1, -1);
     glTexCoord2f(0,height);
     glVertex3f(0, 0, -1);
     glTexCoord2f(width,height);
     glVertex3f(1, 0, -1);
     glTexCoord2f(width,0);
     glVertex3f(1, 1, -1);

     glEnd();
} 
 

void DoBlur(unsigned int uiSrcTex, unsigned int uiTargetTex, int srcTexWidth, int srcTexHeight, int destTexWidth, int destTexHeight, float xofs, float yofs)
{
     cgGLBindProgram(g_cgVP_GlowBlur);
     cgGLBindProgram(g_cgFP_GlowBlur);

     glActiveTextureARB(GL_TEXTURE0_ARB);
     glEnable(GL_TEXTURE_RECTANGLE_NV);
     glBindTexture(GL_TEXTURE_RECTANGLE_NV, uiSrcTex);

     glActiveTextureARB(GL_TEXTURE1_ARB);
     glEnable(GL_TEXTURE_RECTANGLE_NV);
     glBindTexture(GL_TEXTURE_RECTANGLE_NV, uiSrcTex);

     glActiveTextureARB(GL_TEXTURE2_ARB);
     glEnable(GL_TEXTURE_RECTANGLE_NV);
     glBindTexture(GL_TEXTURE_RECTANGLE_NV, uiSrcTex);

     glActiveTextureARB(GL_TEXTURE3_ARB);
     glEnable(GL_TEXTURE_RECTANGLE_NV);
     glBindTexture(GL_TEXTURE_RECTANGLE_NV, uiSrcTex);

     cgGLSetParameter1f(g_cgpVP1_XOffset, xofs);
     cgGLSetParameter1f(g_cgpVP1_YOffset, yofs);

     glViewport(0, 0, destTexWidth, destTexHeight);

     DrawQuad(srcTexWidth, srcTexHeight);
     
     glBindTexture(GL_TEXTURE_RECTANGLE_NV, uiTargetTex);
     glCopyTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB, 0, 0, destTexWidth, destTexHeight, 0);
} 
 

void RenderScreenGlow(void)
{
     if (IEngineStudio.IsHardware() != 1)
          return;

     if (!g_bInitialised)
          InitScreenGlow();

     if ((int)gEngfuncs.pfnGetCvarFloat("ms_bloom_level") == 0)
          return; 
 

     // STEP 1: Grab the screen and put it into a texture

     glActiveTextureARB(GL_TEXTURE0_ARB);
     glEnable(GL_TEXTURE_RECTANGLE_NV);

     glBindTexture(GL_TEXTURE_RECTANGLE_NV, g_uiSceneTex);
     glCopyTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB, 0, 0, ScreenWidth, ScreenHeight, 0); 
 


     // STEP 2: Set up an orthogonal projection

     glMatrixMode(GL_MODELVIEW);
     glPushMatrix();
     glLoadIdentity();
     
     glMatrixMode(GL_PROJECTION);
     glPushMatrix();
     glLoadIdentity();
     glOrtho(0, 1, 1, 0, 0.1, 100);
     
     glColor3f(1,1,1); 
 

     // STEP 3: Initialize Cg programs and parameters for darkening mid to dark areas of the scene

     cgGLEnableProfile(g_cgVertProfile);
     cgGLEnableProfile(g_cgFragProfile);

     cgGLBindProgram(g_cgVP_GlowDarken);
     cgGLBindProgram(g_cgFP_GlowDarken);

     cgGLSetStateMatrixParameter(g_cgpVP0_ModelViewMatrix, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY); 
 

	// STEP 4: Render the current scene texture to a new, lower-res texture, darkening non-bright areas of the scene

    //MiB 31_DEC2010 - Give bloom ability to be darker
    //      Commented out the old way
	/*glViewport(0, 0, ScreenWidth/2, ScreenHeight/2);

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, g_uiSceneTex);

	DrawQuad(ScreenWidth, ScreenHeight);

	glBindTexture(GL_TEXTURE_RECTANGLE_NV, g_uiBlurTex);
	glCopyTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB, 0, 0, ScreenWidth/2, ScreenHeight/2, 0); 
	*/

	// New way
	glViewport(0, 0, ScreenWidth/2, ScreenHeight/2);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	glBindTexture(GL_TEXTURE_RECTANGLE_NV, g_uiSceneTex);

	glBlendFunc(GL_DST_COLOR, GL_ZERO);
	glDisable(GL_BLEND);
	DrawQuad(ScreenWidth, ScreenHeight);
	glEnable(GL_BLEND);

	int darkenLevel = (int)gEngfuncs.pfnGetCvarFloat("ms_bloom_darken");
	if ( darkenLevel == -1 ) darkenLevel = MSCLGlobals::mapDarkenLevel;
	foreach( i , darkenLevel )
		DrawQuad( ScreenWidth, ScreenHeight );

	glBindTexture(GL_TEXTURE_RECTANGLE_NV, g_uiBlurTex);
		glCopyTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGB, 0, 0, ScreenWidth/2, ScreenHeight/2, 0);

	glBlendFunc(GL_SRC_ALPHA, GL_ZERO);
	//End changes
 
     // STEP 5: Initialise Cg programs and parameters for blurring

     cgGLBindProgram(g_cgVP_GlowBlur);
     cgGLBindProgram(g_cgFP_GlowBlur);

     cgGLSetStateMatrixParameter(g_cgpVP1_ModelViewMatrix, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY); 
 


     // STEP 6: Apply blur

     int iNumBlurSteps = (int)gEngfuncs.pfnGetCvarFloat("ms_bloom_level");
     for (int i = 0; i < iNumBlurSteps; i++) {
          DoBlur(g_uiBlurTex, g_uiBlurTex, ScreenWidth/2, ScreenHeight/2, ScreenWidth/2, ScreenHeight/2, 1, 0);     
          DoBlur(g_uiBlurTex, g_uiBlurTex, ScreenWidth/2, ScreenHeight/2, ScreenWidth/2, ScreenHeight/2, 0, 1);
     } 
 


     // STEP 7: Set up Cg for combining blurred glow with original scene
     
     cgGLBindProgram(g_cgVP_GlowCombine);
     cgGLBindProgram(g_cgFP_GlowCombine);
     
     cgGLSetStateMatrixParameter(g_cgpVP2_ModelViewMatrix, CG_GL_MODELVIEW_PROJECTION_MATRIX, CG_GL_MATRIX_IDENTITY);

     glActiveTextureARB(GL_TEXTURE0_ARB);
     glEnable(GL_TEXTURE_RECTANGLE_NV);
     glBindTexture(GL_TEXTURE_RECTANGLE_NV, g_uiSceneTex);

     glActiveTextureARB(GL_TEXTURE1_ARB);
     glEnable(GL_TEXTURE_RECTANGLE_NV);
     glBindTexture(GL_TEXTURE_RECTANGLE_NV, g_uiBlurTex); 
 


     // STEP 8: Do the combination, rendering to the screen without grabbing it to a texture

     glViewport(0, 0, ScreenWidth, ScreenHeight);

     DrawQuad(ScreenWidth/2, ScreenHeight/2); 
 


     // STEP 9: Restore the original projection and modelview matrices and disable rectangular textures on all units
     
     glMatrixMode(GL_PROJECTION);
     glPopMatrix();

     glMatrixMode(GL_MODELVIEW);
     glPopMatrix();

     cgGLDisableProfile(g_cgVertProfile);
     cgGLDisableProfile(g_cgFragProfile);

     glActiveTextureARB(GL_TEXTURE0_ARB);
     glDisable(GL_TEXTURE_RECTANGLE_NV);
     
     glActiveTextureARB(GL_TEXTURE1_ARB);
     glDisable(GL_TEXTURE_RECTANGLE_NV);

     glActiveTextureARB(GL_TEXTURE2_ARB);
     glDisable(GL_TEXTURE_RECTANGLE_NV);

     glActiveTextureARB(GL_TEXTURE3_ARB);
     glDisable(GL_TEXTURE_RECTANGLE_NV);

     glActiveTextureARB(GL_TEXTURE0_ARB);
} 
//[/TWHL]