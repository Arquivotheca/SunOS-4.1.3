/*      @(#)audio_asm.s 1.1 92/07/30 SMI      */ 


#include <machine/intreg.h>
#include <machine/asm_linkage.h>
#include <machine/auxio.h>
#include <machine/mmu.h>

#include <sbusdev/audiovar.h>

#include "assym.s"

#define Aptr	%l3
#define Ring	%l4
#define Tmp1	%l5
#define Tmp2	%l6
#define Tmp3	%l7

/*  The SET_REASON macro sets the specified interrupt bit in the
 *  INTERRUPT_REASON field, then sets a flag so that the level 4
 *  interrupt will be scheduled.  It assumes that Aptr points to
 *  the audio structure and trashes registers Tmp1 and Tmp2.
 */

#define SET_REASON(bit)					\
	ld	[Aptr + AUDIO_INTERRUPT_REASON],Tmp1;	\
	mov	(bit),Tmp2;				\
	or	Tmp1,Tmp2,Tmp1;				\
	st	Tmp1,[Aptr + AUDIO_INTERRUPT_REASON];	\
	mov	1,Tmp2;					\
	sethi	%hi(schedule_flag),Tmp1;		\
	st	Tmp2,[Tmp1 + %lo(schedule_flag)]

#define LED_OFF()					\
	sethi   %hi(AUXIO_REG), Tmp1;			\
        ldub    [Tmp1 + %lo(AUXIO_REG)], Tmp2;		\
        and     Tmp2, ~AUX_LED, Tmp2;			\
        or      Tmp2, AUX_MBO, Tmp2;			\
        stb     Tmp2, [Tmp1 + %lo(AUXIO_REG)]

/*  We declare the variable _audio here so that the references to it
 *  can be resolved even if audio is not configured into the kernel.
 *  Normally it would be declared in audio.c, but the audio is different
 *  because we have a hard wired trap handler.
 */
	.seg	"data"
	.align	4
	.global	_audio
_audio:
	.word	0x0
schedule_flag:
	.word	0x0

	.seg	"text"
	.global	audio_trap

audio_trap:
	/*  Clear schedule_flag.
	 */
	mov	0,Tmp1
	sethi	%hi(schedule_flag),Tmp2
	st	Tmp1,[Tmp2 + %lo(schedule_flag)]

	sethi	%hi(_audio),Aptr		! set Aptr for good
	ld	[Aptr + %lo(_audio)],Aptr

	/*  Read the interrupt register to enable the next interrupt.
	 */
	ld	[Aptr + AUDIO_CHIP],Tmp1	! Tmp1 = aptr->chip
	ldsb	[Tmp1 + AUDIO_CHIP_IR],Tmp2	! Tmp2 = Tmp1->dr

	/*  See if this is a write or a read or unexpected, and branch to
	 *  the appropriate place.
	 */
write_test:
	ld	[Aptr + AUDIO_WRITE_ACTIVE],Tmp1
	tst	Tmp1
	bnz	write_interrupt
	nop
read_test:
	ld	[Aptr + AUDIO_READ_ACTIVE],Tmp1
	tst	Tmp1
	bnz	read_interrupt
	nop
	b	done
	nop

write_interrupt:
	add	Aptr,AUDIO_WRITE_RING,Ring	! point to the write ring

	/*  If there is no data in the ring (head == tail) then
	 *  we stop the i/o.
	 */
	ld	[Ring + AUDIO_RING_HEAD],Tmp1	! Tmp1 = head
	ld	[Ring + AUDIO_RING_TAIL],Tmp2	! Tmp2 = tail
	cmp	Tmp1,Tmp2			!
	bne	w1
	nop

	/*  There is no data in the write ring, so arrange for the
	 *  write to be stopped.
	 */
	SET_REASON(AUDIO_INTERRUPT_STOP_WRITE)
	b	read_test
	nop

w1:
	/*  There is some data left in the ring.  Write it to the device.
	 */
	ld	[Ring + AUDIO_RING_DATA],Tmp1	! point to data
	add	Tmp1,Tmp2,Tmp1			! add tail offset to data ptr
	ldub	[Tmp1],Tmp2			! get the byte from the buffer

	ld	[Aptr + AUDIO_CHIP],Tmp3	! point to chip
	stb	Tmp2,[Tmp3 + AUDIO_CHIP_BBTB]	! copy the data byte to it

	/*  Increment tail to point to the next data location.
	 */
	ld	[Ring + AUDIO_RING_TAIL],Tmp1	! Tmp1 = tail
	add	Tmp1,1,Tmp1			! tail++
	ld	[Ring + AUDIO_RING_MASK],Tmp2	! Tmp2 = mask
	and	Tmp1,Tmp2,Tmp1			! tail &= mask
	st	Tmp1,[Ring + AUDIO_RING_TAIL]	! store the new tail value

	/*  If tail == wakeup, we schedule a wakeup so that the other
	 *  half of the driver is awakened.
	 */
	ld	[Ring + AUDIO_RING_WAKEUP],Tmp2	! wakeup value
	cmp	Tmp1,Tmp2
	bne	read_test
	nop

	/*  Set wakeup to -1 (so we don't keep trying to wake up the other
	 *  half, store the reason for the wakeup, and schedule the interrupt.
	 */
	mov	-1,Tmp1
	st	Tmp1,[Ring + AUDIO_RING_WAKEUP]		! wakeup is now -1
	SET_REASON(AUDIO_INTERRUPT_WAKEUP_WRITE)
	b	read_test
	nop

read_interrupt:
	add	Aptr,AUDIO_READ_RING,Ring	! point to the write ring

	/*  if (((head + 1) & mask) != tail) then there is room to read
	 *  a byte from the device.  If there is no room, then we stop
	 *  the i/o.
	 */
	ld	[Ring + AUDIO_RING_HEAD],Tmp1	! Tmp1 = ring->head
	add	Tmp1,1,Tmp1			! Tmp1++
	ld	[Ring + AUDIO_RING_MASK],Tmp2	! Tmp2 = ring->mask
	and	Tmp1,Tmp2,Tmp1			! Tmp1 &= ring->mask
	ld	[Ring + AUDIO_RING_TAIL],Tmp3	! Tmp3 = ring->tail
	cmp	Tmp1,Tmp3
	bne	r1
	nop

	/*  There is no more room in the buffer, so stop the read.
	 */
	SET_REASON(AUDIO_INTERRUPT_STOP_READ)
	b	done
	nop

	/*  We have room.  Read the byte from data[head].
	 */
r1:
	ld	[Ring + AUDIO_RING_HEAD],Tmp1	! Tmp1 = ring->head
	ld	[Ring + AUDIO_RING_DATA],Tmp2	! point to start of data
	add	Tmp2,Tmp1,Tmp2			! add offset to head
	ld	[Aptr + AUDIO_CHIP],Tmp3	! point to chip
	ldub	[Tmp3 + AUDIO_CHIP_BBRB],Tmp3	! load bbrb register
	stb	Tmp3,[Tmp2]			! store it in data address

	/* Increment head to point to the next address.
	 */
	add	Tmp1,1,Tmp1			! head++
	ld	[Ring + AUDIO_RING_MASK],Tmp2	! get mask value
	and	Tmp1,Tmp2,Tmp1			! mask head value
	st	Tmp1,[Ring + AUDIO_RING_HEAD]	! store the new value for head

	/*  If head == wakeup, the arrange for the other half of the driver
	 *  to be awakened.
	 */
	ld	[Ring + AUDIO_RING_WAKEUP],Tmp2	! wakeup value
	cmp	Tmp1,Tmp2
	bne	done
	nop

	SET_REASON(AUDIO_INTERRUPT_WAKEUP_READ);

	/*  Drop through to done.
	 */

done:
	! %%% It is assumed that the softint was cleared by the
	! interrupt steering logic before this code was called; if
	! this assumption is wrong, add code here to clear the level 4
	! software interrupt if schedule_flag is nonzero.

	mov	%l0, %psr
	nop					! The nop's are mandatory after
	nop					! setting the psr
	jmp	%l1
	rett	%l2
	.empty



