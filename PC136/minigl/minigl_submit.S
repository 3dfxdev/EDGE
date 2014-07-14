
.align 2
.globl _dcglLoadTransposeMatrix
_dcglLoadTransposeMatrix:
	frchg
	fmov @r4+,fr0
	fmov @r4+,fr4
	fmov @r4+,fr8
	fmov @r4+,fr12
	fmov @r4+,fr1
	fmov @r4+,fr5
	fmov @r4+,fr9
	fmov @r4+,fr13
	fmov @r4+,fr2
	fmov @r4+,fr6
	fmov @r4+,fr10
	fmov @r4+,fr14
	fmov @r4+,fr3
	fmov @r4+,fr7
	fmov @r4+,fr11
	fmov @r4+,fr15
	rts
	frchg
	
.align 2
.globl _dcglStoreTransposeMatrix
_dcglStoreTransposeMatrix:
	frchg
	add #64,r4
	fmov fr15,@-r4
	fmov fr11,@-r4
	fmov fr7,@-r4
	fmov fr3,@-r4
	fmov fr14,@-r4
	fmov fr10,@-r4
	fmov fr6,@-r4
	fmov fr2,@-r4
	fmov fr13,@-r4
	fmov fr9,@-r4
	fmov fr5,@-r4
	fmov fr1,@-r4
	fmov fr12,@-r4
	fmov fr8,@-r4
	fmov fr4,@-r4
	fmov fr0,@-r4
	rts
	frchg

#define srcp		r4
#define srcstride	r5
#define dstp		r6
#define dststride	r7
#define count		r1
/*
void dcglTransformVectors(float *srcp, size_t srcstride, float *dstp, size_t dststride, size_t count) {
	srcstride /= sizeof(*srcp);
	dststride /= sizeof(*dstp);
	do {
		float x = srcp[0], y = scrp[1], z = scrp[2], w = scrp[3];
		ftrv(x,y,z,w);
		srcp += srcstride;
		dstp[0] = x; dstp[1] = y; dstp[2] = z; dstp[3] = w;
		dstp += dststride;
	} while(--count);
}
*/
.align 5
.globl _dcglTransformVectorsMat
_dcglTransformVectorsMat:
	fschg
	mov.l @(4,r15),r0
	
	mov.l @r15,count
	fmov @r0+,xd0
	fmov @r0+,xd2
	fmov @r0+,xd4
	fmov @r0+,xd6
	fmov @r0+,xd8
	fmov @r0+,xd10
	fmov @r0+,xd12
	bra 1f
	fmov @r0+,xd14

.align 5
.globl _dcglTransformVectors
_dcglTransformVectors:
	fschg			
	mov.l @r15,count
1:
	fmov @srcp,dr0		
	mov #8,r0
	
	fmov @(r0,srcp),dr2	
	add srcstride,srcp
	
	sub dststride,dstp
	
	!
	.align 2
1:
	ftrv xmtrx,fv0
	dt count
	
	fmov @srcp,dr4
	bt/s 2f
	
	add dststride,dstp
	fmov @(r0,srcp),dr6
	
	add srcstride,srcp
	
	!
	
	fmov dr0,@dstp
	ftrv xmtrx,fv4
	
	fmov dr2,@(r0,dstp)
	add dststride,dstp
	
	fmov @srcp,dr0
	dt count
	
	fmov @(r0,srcp),dr2
	add srcstride,srcp
	
	!
	
	fmov dr4,@dstp
	bf/s	1b
	fmov dr6,@(r0,dstp)
	
	rts
	
	fschg
2:
	fmov dr0,@dstp
	
	fmov dr2,@(r0,dstp)
	
	rts
	fschg
#undef srcp
#undef srcstride
#undef dstp
#undef dststride
#undef count
//


/*
int dcglAnyGreater(float valcompare, float *val, size_t stride, int count) {
//dcgl_needs_clipping |= pos.w < dcgl_prim.nearclip;
	stride /= sizeof(*val);
	do {
		if (valcompare > *val)
			return 1;
		val += stride;
	} while(--count);
	return 0;
}
*/

#define nearz 	fr4
#define zptr 	r4
#define stride 	r5
#define count 	r6
.align 5
.globl _dcglAnyGreater
_dcglAnyGreater:
	shlr		count
	fmov		@zptr,fr1
	
	add		stride,zptr
	bf		1f
		
	tst		count,count
	
	bt/s		.Lreturntbit
	!fcmp/gt 	nearz,fr1
	fcmp/gt 	fr1,nearz
	
	!
	
	bt		.Lneedclipping
1:	shlr		count
	
	bf		2f
	tst		count,count

	bt/s		.Lnoneleft2
	!fcmp/gt 	nearz,fr1
	fcmp/gt 	fr1,nearz
	
	fmov		@zptr,fr0
	add		stride,zptr	
	
	bt		.Lneedclipping
	
	!fcmp/gt 	nearz,fr0
	fcmp/gt 	fr0,nearz
	
	fmov		@zptr,fr1
	add		stride,zptr
	
	bt		.Lneedclipping
.align 2
2:	
	
	fmov		@zptr,fr0
	add		stride,zptr

	!fcmp/gt 	nearz,fr1
	fcmp/gt 	fr1,nearz
.align 2
3:
	fmov		@zptr,fr1
	add		stride,zptr
	
	bt		.Lneedclipping
	!fcmp/gt 	nearz,fr0
	fcmp/gt 	fr0,nearz
	
	fmov		@zptr,fr0
	add		stride,zptr
	
	bt		.Lneedclipping
	!fcmp/gt 	nearz,fr1
	fcmp/gt 	fr1,nearz
	
	fmov		@zptr,fr1
	add		stride,zptr
	
	bt		.Lneedclipping
	!fcmp/gt 	nearz,fr0
	fcmp/gt 	fr0,nearz
	
	fmov		@zptr,fr0
	add		stride,zptr
	
	bt		.Lneedclipping
	dt		count
	
	bf/s		3b
	!fcmp/gt 	nearz,fr1
	fcmp/gt 	fr1,nearz
	
	rts
	mov		#0,r0
	
.Lneedclipping:
	rts
	mov		#1,r0
	
.Lnoneleft2:
	fmov		@zptr,fr0
	bt		.Lreturntbit
	
	!fcmp/gt 	nearz,fr0
	fcmp/gt 	fr0,nearz
.Lreturntbit:
	rts
	movt		r0
	
#undef nearz
#undef zptr
#undef stride
#undef count

/*
void * dcglSubmitMany(float *pos, float *uv, float *argb, int stride, int striplen, int count, void *dst);

void * dcglSubmitMany(float *pos, float *uv, float *argb, int stride, int striplen, int count, void *dst) {
	int tempstriplen = striplen;
	do {
		float invz = 1.0f / pos[3];
		if (--tempstriplen) {
			nv->cmd = vert;
		} else {
			nv->cmd = eos;
			tempstriplen = striplen;
		}
		nv->v = uv[1];
		nv->u = uv[0];
		nv->z = invz;
		nv->y = pos[1] * invz;
		nv->x = pos[0] * invz;
		
		pref(nv);
		nv += 32;
		
		nv->fbo = vert->fbo;
		nv->fgo = vert->fgo;
		nv->fro = vert->fro;
		nv->fog = vert->fog;
		nv->fb = vert->fb;
		nv->fg = vert->fg;
		nv->fr = vert->fr;
		nv->fa = vert->fa;
		
		pref(nv);
		nv += 32;
	} while(--count);
	return nv;
}
*/
#define pos		r4
#define uvp		r5
#define color		r6
#define stride		r7
#define posstride	r7
#define uvstride	r7
#define colorstride	r7
#define striplen	r2
#define strip		r3
#define dst		r1
#define cmddst		r9
#define cmd_vert	r10
#define cmd_vert_eos	r11
#define count		r8
#define w		fr3
#define z		fr2

.align 5
.globl _dcglSubmitPerspective
_dcglSubmitPerspective:
	mov.l	count,@-r15
	
	mov.l	cmddst,@-r15
	fschg
	
	fmov	@pos+,dr0
	
	fmov	@pos,dr2
	add	posstride,pos
	
	mov	#8,r0	
	fldi1	z
	
	fdiv	w,z
	mov.l	@(8+2*4,r15),dst
	
	mov.l	@(8+0,r15),striplen
	add	#-8,pos
	
	mov	dst,cmddst
	add	#16,dst
	
	mov.l	@(8+1*4,r15),count
	mov	striplen,strip
	
	mov.l	cmd_vert_eos,@-r15
	
	mov.l	cmd_vert,@-r15
	
	mov.l	.Lsp_cmd_vert,cmd_vert
	
	mov.l	.Lsp_cmd_vert_eos,cmd_vert_eos
	
	dt	strip
	mov.l	cmd_vert_eos,@cmddst	!write vertcmd
	
	fmov	@uvp,dr4
	add	uvstride,uvp
	
	bf	1f	
	mov	striplen,strip
1:
	fmov	dr4,@dst	!write uv
	fschg
	
	bt	1f
	mov.l	cmd_vert,@cmddst	!write eoscmd
1:
	dt	count
	fmul	z,fr1
	
	fmul	z,fr0
	fmov	z,@-dst		!write 1/w
	
	fmov	fr1,@-dst	!write y/w
	
	fmov	fr0,@-dst	!write x/w
	fschg
	
	pref	@dst		!submit
	bt/s	.Lsp_one
	
	fmov	@pos+,dr0
	
.align 2
.Lsp_loop:
	fmov	@pos,dr2
	add	posstride,pos
	
	fmov	@color+,dr4
	add	#-8,pos
	
	fldi1	z
	fdiv	w,z

	fmov	@color+,dr6
	dt	strip
	
	fmov	@(r0,color),dr10
	bf	1f
	
	mov	striplen,strip
1:
	fmov	@color,dr8
	add	#64-4-8,dst
	
	fmov	dr10,@dst	!write go,bo
	
	add	#64,cmddst
	fmov	dr8,@-dst	!write fog,ro
	
	fmov	dr6,@-dst	!write g,b
	
	fmov	dr4,@-dst	!write a,r
	add	colorstride,color
	
	pref	@dst		!submit
	add	#64-16,dst
	
	mov.l	cmd_vert_eos,@cmddst
	add	#-16,color
	
	fmov	@uvp,dr4
	add	uvstride,uvp
	
	bt	1f
	mov.l	cmd_vert,@cmddst
1:
	fmov	dr4,@dst	!write uv
	fschg
	
	fmul	z,fr1
	dt	count
	
	fmul	z,fr0
	fmov	z,@-dst		!write 1/w
	
	fmov	fr1,@-dst	!write y/w
	
	fmov	fr0,@-dst	!write x/w
	fschg
	
	pref	@dst
	bf/s	.Lsp_loop
	fmov	@pos+,dr0

.Lsp_one:
	fmov	@color+,dr4
	
	fmov	@color+,dr6
	
	add	#64-4-8,dst
	fmov	@(r0,color),dr10
	
	fmov	@color,dr8
	
	fmov	dr10,@dst	!write go,bo
	
	fmov	dr8,@-dst	!write fog,ro
	
	fmov	dr6,@-dst	!write g,b
	mov	cmddst,r0
	
	fmov	dr4,@-dst	!write a,r
	add	#64,r0
	
	pref	@dst		!submit
	fschg
	
	mov.l	@r15+,cmd_vert
	
	mov.l	@r15+,cmd_vert_eos
	
	mov.l	@r15+,cmddst
	
	rts
	mov.l	@r15+,count

#undef pos
#undef uvp
#undef color
#undef stride
#undef posstride
#undef uvstride
#undef colorstride
#undef striplen
#undef strip
#undef dst
#undef cmddst
#undef cmd_vert
#undef cmd_vert_eos
#undef count
#undef w
#undef z

/*
void dcglSubmitIndexedUBC(DCGLpvrvertex *v, unsigned char *idx, int count, void *dst, int restartidx) {
	while(count--) {
		if (*idx == restartidx)
			submit_eos(v+*idx);
		else
			submit(v+*idx);
		idx++;
	}
}
*/

#define shiftamt r1
#define cmd_vert_eos r2
#define restartidx r3
#define src r4
#define idxp r5
#define count r6
#define dst r7
#define idx2 r8
#define idx3 r9
#define idx r10
.align 5
.globl _dcglSubmitIndexedUB
_dcglSubmitIndexedUB:
	mov	#8,r0
	mov.l	r8,@-r15

	mov.l	r9,@-r15
	add	#-64,dst
	
	mov.l	r10,@-r15
	
	mov	#6,shiftamt
	mov.b	@idxp+,idx
	
	dt	count
	mov.l	.Lsp_cmd_vert_eos,cmd_vert_eos
	
	extu.b	idx,idx
	bt/s	.Lsiub_end
	
	fschg
	shld	shiftamt,idx

	add	src,idx
	mov.l	@(4*3,r15),restartidx
	
.Lsiub_loop:
	mov.b	@idxp+,idx2
	
	fmov	@idx+,dr0
	add	#64+32-8,dst
	
	fmov	@idx+,dr2
	mov	idx2,idx3
	
	fmov	@idx,dr6
	extu.b	idx2,idx2
	
	fmov	@(r0,idx),dr4
	cmp/eq 	idx2,restartidx
	
	fmov	dr6,@dst
	
	fmov	dr4,@-dst
	
	fmov	dr2,@-dst
	
	bt/s	.Lsiub_restart
	fmov	dr0,@-dst
	
2:
	pref	@dst
	add	#32+32-8,dst
	
	fmov	@idx+,dr0
	extu.b	idx3,idx3
	
	fmov	@idx+,dr2
	shld	shiftamt,idx3
	
	fmov	@idx,dr6
	add	src,idx3
	
	fmov	@(r0,idx),dr4
	
	fmov	dr6,@dst
	
	fmov	dr4,@-dst
	mov	idx3,idx
	
	fmov	dr2,@-dst
	dt	count
	
	fmov	dr0,@-dst
	bf/s 	.Lsiub_loop
	
	pref	@dst
	
.Lsiub_end:
	mov.l	@r15+,r10
	mov.l	@r15+,r9
	mov.l	@r15+,r8
	rts
	fschg
	
.align 2
.Lsiub_restart:
	mov.b	@idxp+,idx3
	bra	2b
	
	mov.l	cmd_vert_eos,@dst
	
.align 3
.Lsp_cmd_vert:
	.long	0xe0000000
.Lsp_cmd_vert_eos:
	.long	0xf0000000