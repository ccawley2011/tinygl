#ifndef _tgl_zbuffer_h_
#define _tgl_zbuffer_h_

/*
 * Z buffer
 */

#include "zfeatures.h"
#include "GL/gl.h"
//#warning stdio is left in!!! remove it!!!
//#include <stdio.h>
#define ZB_Z_BITS 16

#define ZB_POINT_Z_FRAC_BITS 14
//a "1" in bit FRAC_BITS+1 (starting at zero) = 1.

//This is a complicated as hell fixed point math standard.

//go to zfeatures.h to find out how this stuff is decided
#define ZB_POINT_S_MIN ( (1<<ZB_POINT_S_FRAC_BITS) )
#define ZB_POINT_S_MAX ( (1<<(1+TGL_FEATURE_TEXTURE_POW2+ZB_POINT_S_FRAC_BITS))-ZB_POINT_S_MIN )
#define ZB_POINT_T_MIN ( (1<<ZB_POINT_T_FRAC_BITS) )
#define ZB_POINT_T_MAX ( (1<<(1+TGL_FEATURE_TEXTURE_POW2+ZB_POINT_T_FRAC_BITS))-ZB_POINT_T_MIN )
#define ZB_POINT_S_VALUE (ZB_POINT_S_FRAC_BITS + 4)
#define ZB_POINT_T_VALUE (ZB_POINT_T_FRAC_BITS - 4)
#define ZB_S_MASK ((TGL_FEATURE_TEXTURE_DIM-1)<<(ZB_POINT_S_FRAC_BITS+1))
#define ZB_T_MASK ((TGL_FEATURE_TEXTURE_DIM-1)<<(ZB_POINT_T_FRAC_BITS+1))
//PSZSH is 5 at 32 bit, or 4 at 16 bit.
#if ZB_POINT_T_FRAC_BITS == (ZB_POINT_S_FRAC_BITS + TGL_FEATURE_TEXTURE_POW2)
#define ST_TO_TEXTURE_BYTE_OFFSET(s,t) ( ((s & ZB_S_MASK) | (t & ZB_T_MASK)) >> (ZB_POINT_S_VALUE-PSZSH))
#else
#define ST_TO_TEXTURE_BYTE_OFFSET(s,t) ( ((s & ZB_S_MASK)>>(ZB_POINT_S_VALUE-PSZSH)) | ((t & ZB_T_MASK)>>(ZB_POINT_T_VALUE-PSZSH))  )
#endif

//This is how textures are sampled. if you want to do some sort of fancy texture filtering,
//you do it here.
#define TEXTURE_SAMPLE(texture, s, t)														\
 (*(PIXEL*)( (GLbyte*)texture + 															\
 ST_TO_TEXTURE_BYTE_OFFSET(s,t) 								\
 ))
/* display modes */
#define ZB_MODE_5R6G5B  1  /* true color 16 bits */
#define ZB_MODE_INDEX   2  /* color index 8 bits */
#define ZB_MODE_RGBA    3  /* 32 bit ABGR mode */
#define ZB_MODE_RGB24   4  /* 24 bit rgb mode */
#define ZB_NB_COLORS    225 /* number of colors for 8 bit display */





#if TGL_FEATURE_RENDER_BITS == 32

/* 32 bit mode */
//#define RGB_TO_PIXEL(r,g,b) ( ((b&65280)<<8) | ((g&65280)) | ((r&65280)>>8) )
#define RGB_TO_PIXEL(r,g,b) \
  ((((r) << 8) & 0xff0000) | ((g) & 0xff00) | ((b & 0xff00) >> 8))
#define GET_RED(p) ((p & 0xff0000)>>16)
#define GET_REDDER(p) ((p & 0xff0000)>>8)
#define GET_GREEN(p) ((p & 0xff00)>>8)
#define GET_GREENER(p) ((p & 0xff00))
#define GET_BLUE(p) (p & 0xff)
#define GET_BLUEER(p) ((p & 0xff)<<8)
typedef GLuint PIXEL;
#define PSZB 4
#define PSZSH 5

#elif TGL_FEATURE_RENDER_BITS == 16

/* 16 bit mode */
#define RGB_TO_PIXEL(r,g,b) (((r) & 0xF800) | (((g & 0xff00) >> 5) & 0x07E0) | ((b & 0xff00) >> 11))

#define GET_RED(p) ((p & 0xF800)>>8)
#define GET_REDDER(p) ((p & 0xF800))

#define GET_GREEN(p) ((p & 0x07E0)>>3)
#define GET_GREENER(p) ((p & 0x07E0)<<5)

#define GET_BLUE(p) ((p & 31)<<3)
#define GET_BLUEER(p) ((p & 31)<<11)

typedef GLushort PIXEL;
#define PSZB 2 
#define PSZSH 4 


#else

#error "wrong TGL_FEATURE_RENDER_BITS buddy"

#endif

#if TGL_FEATURE_LIT_TEXTURES == 1
#define RGB_MIX_FUNC(rr, gg, bb, tpix) \
	RGB_TO_PIXEL( \
		((rr * GET_RED(tpix))>>8),\
		((gg * GET_GREEN(tpix))>>8),\
		((bb * GET_BLUE(tpix))>>8)\
	)
#else
#define RGB_MIX_FUNC(rr, gg, bb, tpix)(tpix)
#endif



#if TGL_FEATURE_BLEND == 1
#define TGL_BLEND_VARS GLuint zbblendeq = zb->blendeq; GLuint sfactor = zb->sfactor; GLuint dfactor = zb->dfactor;
#define TGL_NO_BLEND_FUNC(source, dest){dest = source;}
#define TGL_NO_BLEND_FUNC_RGB(rr, gg, bb, dest){dest = RGB_TO_PIXEL(rr,gg,bb);}
//SORCERY to achieve 32 bit signed integer clamping
#define TGL_CLAMPI(imp) ( (imp>0) * (65535 * (imp>65535) + imp * (!(imp>65535)) )      )

#define TGL_BLEND_SWITCH_CASE(sr,sg,sb,dr,dg,db,dest) 									\
		switch(zbblendeq){															\
			case GL_FUNC_ADD:															\
			default:																	\
				sr+=dr;sg+=dg;sb+=db;													\
				sr = TGL_CLAMPI(sr);													\
				sg = TGL_CLAMPI(sg);													\
				sb = TGL_CLAMPI(sb);													\
				dest = RGB_TO_PIXEL(sr,sg,sb);											\
			break;																		\
			case GL_FUNC_SUBTRACT:														\
				sr-=dr;sg-=dg;sb-=db;													\
				sr = TGL_CLAMPI(sr);													\
				sg = TGL_CLAMPI(sg);													\
				sb = TGL_CLAMPI(sb);													\
				dest = RGB_TO_PIXEL(sr,sg,sb);											\
			break;																		\
			case GL_FUNC_REVERSE_SUBTRACT:												\
				sr=dr-sr;sg=dg-sg;sb=db-sb;												\
				sr = TGL_CLAMPI(sr);													\
				sg = TGL_CLAMPI(sg);													\
				sb = TGL_CLAMPI(sb);													\
				dest = RGB_TO_PIXEL(sr,sg,sb);											\
			break;																		\
			  																			\
		}




#define TGL_BLEND_FUNC(source, dest){													\
	{																					\
	GLuint sr, sg, sb, dr, dg, db;														\
	{	GLuint t = source;																\
	sr = GET_REDDER(t); sg = GET_GREENER(t); sb = GET_BLUEER(t);						\
	t = dest;																			\
	dr = GET_REDDER(t); dg = GET_GREENER(t); db = GET_BLUEER(t);}						\
		/*printf("\nShould never reach this point!");*/									\
		switch(sfactor){															\
			case GL_ONE:																\
			default:																	\
			break;																		\
			case GL_ONE_MINUS_SRC_COLOR:												\
			sr = ~sr & 0xffff;															\
			sg = ~sg & 0xffff;															\
			sb = ~sb & 0xffff;															\
			break;																		\
			case GL_ZERO:																\
			sr=0;sg=0;sb=0;break;														\
			break;																		\
		}																				\
		switch(dfactor){															\
				case GL_ONE:															\
				default:																\
				break;																	\
				case GL_ONE_MINUS_DST_COLOR:											\
				dr = ~dr & 0xffff;														\
				dg = ~dg & 0xffff;														\
				db = ~db & 0xffff;														\
				break;																	\
				case GL_ZERO:															\
				dr=0;dg=0;db=0;break;													\
				break;																	\
			}																			\
		TGL_BLEND_SWITCH_CASE(sr,sg,sb,dr,dg,db,dest)									\
	}																					\
} ///////////////////////////////////////////////////////////////////////////////////////

#define TGL_BLEND_FUNC_RGB(rr, gg, bb, dest){											\
	{																					\
		GLint sr = rr & 0xFFFF, sg = gg & 0xFFFF, sb = bb & 0xFFFF, dr, dg, db;			\
		{GLuint t = dest;																\
		dr = GET_REDDER(t); dg = GET_GREENER(t); db = GET_BLUEER(t);}					\
	/*printf("\nShould never reach this point!");*/										\
		switch(sfactor){																\
			case GL_ONE:																\
			default:																	\
			break;																		\
			case GL_ONE_MINUS_SRC_COLOR:												\
			sr = ~sr & 0xffff;															\
			sg = ~sg & 0xffff;															\
			sb = ~sb & 0xffff;															\
			break;																		\
			case GL_ZERO:																\
			sr=0;sg=0;sb=0;break;														\
			break;																		\
		}																				\
		switch(dfactor){															\
				case GL_ONE:															\
				default:																\
				break;																	\
				case GL_ONE_MINUS_DST_COLOR:											\
				dr = ~dr & 0xffff;														\
				dg = ~dg & 0xffff;														\
				db = ~db & 0xffff;														\
				break;																	\
				case GL_ZERO:															\
				dr=0;dg=0;db=0;break;													\
				break;																	\
		}																				\
		TGL_BLEND_SWITCH_CASE(sr,sg,sb,dr,dg,db,dest)									\
	}																					\
} ///////////////////////////////////////////////////////////////////////////////////////

#else
#define TGL_BLEND_VARS /* a comment */
#define TGL_BLEND_FUNC(source, dest){dest = source;}
#define TGL_BLEND_FUNC_RGB(rr, gg, bb, dest){dest = RGB_TO_PIXEL(rr,gg,bb);}
#endif


typedef struct {
    GLint xsize,ysize;
    GLint linesize; /* line size, in bytes */
    GLint mode;
    
    GLushort *zbuf;
    PIXEL *pbuf;
    GLint frame_buffer_allocated;
    /* depth */
    GLint depth_test;
    GLint depth_write;
	/* point size*/
    GLfloat pointsize;

//    GLint nb_colors;
//    unsigned char *dctable;
//    GLint *ctable;
    PIXEL *current_texture;
    /* opengl polygon stipple*/

#if TGL_FEATURE_POLYGON_STIPPLE == 1
    GLubyte stipplepattern[TGL_POLYGON_STIPPLE_BYTES];
    GLuint dostipple;
#endif
	GLenum blendeq, sfactor, dfactor;
    GLint enable_blend;
} ZBuffer;

typedef struct {
  GLint x,y,z;     /* integer coordinates in the zbuffer */
  GLint s,t;       /* coordinates for the mapping */
  GLint r,g,b;     /* color indexes */
  
  GLfloat sz,tz;   /* temporary coordinates for mapping */
} ZBufferPoint;

/* zbuffer.c */

ZBuffer *ZB_open(int xsize,int ysize,int mode,
//		 GLint nb_colors,
//		 unsigned char *color_indexes,
//		 GLint *color_table,
		 void *frame_buffer);


void ZB_close(ZBuffer *zb);

void ZB_resize(ZBuffer *zb,void *frame_buffer,GLint xsize,GLint ysize);
void ZB_clear(ZBuffer *zb,GLint clear_z,GLint z,
	      GLint clear_color,GLint r,GLint g,GLint b);
/* linesize is in BYTES */
void ZB_copyFrameBuffer(ZBuffer *zb,void *buf,GLint linesize);

/* zdither.c */

/*
void ZB_initDither(ZBuffer *zb,GLint nb_colors,
		   unsigned char *color_indexes,GLint *color_table);
void ZB_closeDither(ZBuffer *zb);
void ZB_ditherFrameBuffer(ZBuffer *zb,unsigned char *dest,
			  GLint linesize);
*/
/* zline.c */

void ZB_plot(ZBuffer *zb,ZBufferPoint *p);
void ZB_line(ZBuffer *zb,ZBufferPoint *p1,ZBufferPoint *p2);
void ZB_line_z(ZBuffer * zb, ZBufferPoint * p1, ZBufferPoint * p2);

/* ztriangle.c */

void ZB_setTexture(ZBuffer *zb, PIXEL *texture);

void ZB_fillTriangleFlat(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleFlatNOBLEND(ZBuffer *zb,
		 ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);


void ZB_fillTriangleSmooth(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);

void ZB_fillTriangleSmoothNOBLEND(ZBuffer *zb,
		   ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
/*
This function goes unused and is removed by Gek.
void ZB_fillTriangleMapping(ZBuffer *zb,
		    ZBufferPoint *p1,ZBufferPoint *p2,ZBufferPoint *p3);
*/
void ZB_fillTriangleMappingPerspective(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);


void ZB_fillTriangleMappingPerspectiveNOBLEND(ZBuffer *zb,
                    ZBufferPoint *p0,ZBufferPoint *p1,ZBufferPoint *p2);

typedef void (*ZB_fillTriangleFunc)(ZBuffer  *,
	    ZBufferPoint *,ZBufferPoint *,ZBufferPoint *);

/* memory.c */
#if TGL_FEATURE_CUSTOM_MALLOC == 1
void gl_free(void *p);
void *gl_malloc(GLint size);
void *gl_zalloc(GLint size);
#else
#include<string.h>
#include<stdlib.h>
inline void gl_free(void* p) { free(p); }
inline void* gl_malloc(GLint size) { return malloc(size); }
inline void* gl_zalloc(GLint size) { return calloc(1, size); }
#endif
void gl_memcpy(void* dest, void* src, GLuint size);

#endif /* _tgl_zbuffer_h_ */
