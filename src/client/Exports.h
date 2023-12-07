#pragma once

#include "Platform.h"

// From hl_weapons
void HUD_PostRunCmd( struct local_state_s *from, struct local_state_s *to, struct usercmd_s *cmd, int runfuncs, double time, unsigned int random_seed );

// From cdll_int
int Initialize( cl_enginefunc_t *pEnginefuncs, int iVersion );
int HUD_VidInit( void );
void HUD_Init( void );
int HUD_Redraw( float flTime, int intermission );
int HUD_UpdateClientData( client_data_t *cdata, float flTime );
void HUD_Reset ( void );
void HUD_PlayerMove( struct playermove_s *ppmove, int server );
void HUD_PlayerMoveInit( struct playermove_s *ppmove );
char HUD_PlayerMoveTexture( char *name );
int HUD_ConnectionlessPacket( const struct netadr_s *net_from, const char *args, char *response_buffer, int *response_buffer_size );
int HUD_GetHullBounds( int hullnumber, float *mins, float *maxs );
void HUD_Frame( double time );
void HUD_VoiceStatus(int entindex, qboolean bTalking);
void HUD_DirectorMessage( int iSize, void *pbuf );
void HUD_ChatInputPosition( int *x, int *y );

// From demo
void Demo_ReadBuffer( int size, unsigned char *buffer );

// From entity
int HUD_AddEntity( int type, struct cl_entity_s *ent, const char *modelname );
void HUD_CreateEntities( void );
void HUD_StudioEvent( const struct mstudioevent_s *event, const struct cl_entity_s *entity );
void HUD_TxferLocalOverrides( struct entity_state_s *state, const struct clientdata_s *client );
void HUD_ProcessPlayerState( struct entity_state_s *dst, const struct entity_state_s *src );
void HUD_TxferPredictionData ( struct entity_state_s *ps, const struct entity_state_s *pps, struct clientdata_s *pcd, const struct clientdata_s *ppcd, struct weapon_data_s *wd, const struct weapon_data_s *pwd );
void HUD_TempEntUpdate( double frametime, double client_time, double cl_gravity, struct tempent_s **ppTempEntFree, struct tempent_s **ppTempEntActive, int ( *Callback_AddVisibleEntity )( struct cl_entity_s *pEntity ), void ( *Callback_TempEntPlaySound )( struct tempent_s *pTemp, float damp ) );
struct cl_entity_s *HUD_GetUserEntity( int index );

// From in_camera
void CAM_Think( void );
int CL_IsThirdPerson( void );
void CL_CameraOffset( float *ofs );

// From input
struct kbutton_s *KB_Find( const char *name );
void CL_CreateMove ( float frametime, struct usercmd_s *cmd, int active );
void HUD_Shutdown( void );
int HUD_Key_Event( int eventcode, int keynum, const char *pszCurrentBinding );

// From inputw32
void Mouse_Activate( void );
void Mouse_Deactivate( void );
void Mouse_Event (int mstate);
void Mouse_Accumulate (void);
void Mouse_ClearStates (void);

// From tri
void HUD_DrawNormalTriangles( void );
void HUD_DrawTransparentTriangles( void );

// From view
void V_CalcRefdef( struct ref_params_s *pparams );

// From GameStudioModelRenderer
int HUD_GetStudioModelInterface( int version, struct r_studio_interface_s **ppinterface, struct engine_studio_api_s *pstudio );
