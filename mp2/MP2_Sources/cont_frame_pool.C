/*
 File: ContFramePool.C
 
 Author:
 Date  : 
 
 */

/*--------------------------------------------------------------------------*/
/* 
 POSSIBLE IMPLEMENTATION
 -----------------------

 The class SimpleFramePool in file "simple_frame_pool.H/C" describes an
 incomplete vanilla implementation of a frame pool that allocates 
 *single* frames at a time. Because it does allocate one frame at a time, 
 it does not guarantee that a sequence of frames is allocated contiguously.
 This can cause problems.
 
 The class ContFramePool has the ability to allocate either single frames,
 or sequences of contiguous frames. This affects how we manage the
 free frames. In SimpleFramePool it is sufficient to maintain the free 
 frames.
 In ContFramePool we need to maintain free *sequences* of frames.
 
 This can be done in many ways, ranging from extensions to bitmaps to 
 free-lists of frames etc.
 
 IMPLEMENTATION:
 
 One simple way to manage sequences of free frames is to add a minor
 extension to the bitmap idea of SimpleFramePool: Instead of maintaining
 whether a frame is FREE or ALLOCATED, which requires one bit per frame, 
 we maintain whether the frame is FREE, or ALLOCATED, or HEAD-OF-SEQUENCE.
 The meaning of FREE is the same as in SimpleFramePool. 
 If a frame is marked as HEAD-OF-SEQUENCE, this means that it is allocated
 and that it is the first such frame in a sequence of frames. Allocated
 frames that are not first in a sequence are marked as ALLOCATED.
 
 NOTE: If we use this scheme to allocate only single frames, then all 
 frames are marked as either FREE or HEAD-OF-SEQUENCE.
 
 NOTE: In SimpleFramePool we needed only one bit to store the state of 
 each frame. Now we need two bits. In a first implementation you can choose
 to use one char per frame. This will allow you to check for a given status
 without having to do bit manipulations. Once you get this to work, 
 revisit the implementation and change it to using two bits. You will get 
 an efficiency penalty if you use one char (i.e., 8 bits) per frame when
 two bits do the trick.
 
 DETAILED IMPLEMENTATION:
 
 How can we use the HEAD-OF-SEQUENCE state to implement a contiguous
 allocator? Let's look a the individual functions:
 
 Constructor: Initialize all frames to FREE, except for any frames that you 
 need for the management of the frame pool, if any.
 
 get_frames(_n_frames): Traverse the "bitmap" of states and look for a 
 sequence of at least _n_frames entries that are FREE. If you find one, 
 mark the first one as HEAD-OF-SEQUENCE and the remaining _n_frames-1 as
 ALLOCATED.

 release_frames(_first_frame_no): Check whether the first frame is marked as
 HEAD-OF-SEQUENCE. If not, something went wrong. If it is, mark it as FREE.
 Traverse the subsequent frames until you reach one that is FREE or 
 HEAD-OF-SEQUENCE. Until then, mark the frames that you traverse as FREE.
 
 mark_inaccessible(_base_frame_no, _n_frames): This is no different than
 get_frames, without having to search for the free sequence. You tell the
 allocator exactly which frame to mark as HEAD-OF-SEQUENCE and how many
 frames after that to mark as ALLOCATED.
 
 needed_info_frames(_n_frames): This depends on how many bits you need 
 to store the state of each frame. If you use a char to represent the state
 of a frame, then you need one info frame for each FRAME_SIZE frames.
 
 A WORD ABOUT RELEASE_FRAMES():
 
 When we releae a frame, we only know its frame number. At the time
 of a frame's release, we don't know necessarily which pool it came
 from. Therefore, the function "release_frame" is static, i.e., 
 not associated with a particular frame pool.
 
 This problem is related to the lack of a so-called "placement delete" in
 C++. For a discussion of this see Stroustrup's FAQ:
 http://www.stroustrup.com/bs_faq2.html#placement-delete
 
 */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/* DEFINES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* INCLUDES */
/*--------------------------------------------------------------------------*/

#include "cont_frame_pool.H"
#include "console.H"
#include "utils.H"
#include "assert.H"



ContFramePool::FrameState ContFramePool::get_state(unsigned long _frame_no){
    unsigned int bitmap_index = _frame_no / 4;
    unsigned char ans = bitmap[bitmap_index] >> ((_frame_no % 4) * 2);
    unsigned char mask = 0x3;

    switch((ans & mask)){
        case 0x0:
        return FrameState::Used;
        case 0x1:
        return FrameState::Free;
        case 0x2:
        return FrameState::HoS;
        default:
        return FrameState::Used;
    }
}

void ContFramePool::set_state(unsigned long _frame_no, FrameState _state){
    unsigned int bitmap_index = _frame_no / 4;
    unsigned char mask1 = 0x3 << ((_frame_no % 4) * 2);
    unsigned char mask2;
    bitmap[bitmap_index] |= mask1;

    switch(_state) {
        case FrameState::Used:
        mask2 = 0x3 << ((_frame_no % 4) * 2);
        break;
        case FrameState::Free:
        mask2 = 0x2 << ((_frame_no % 4) * 2);
        break;
        case FrameState::HoS:
        mask2 = 0x1 << ((_frame_no % 4) * 2);
    }
    bitmap[bitmap_index] ^= mask2;
    
}

void ContFramePool::release_frame_in_pool(unsigned long _first_frame_no){
    if(get_state(_first_frame_no-base_frame_no)!=FrameState::HoS){
        return;
    }
    set_state(_first_frame_no-base_frame_no,FrameState::Free);
    nFreeFrames++;
    int fno = _first_frame_no-base_frame_no+1;
    while(get_state(fno)==FrameState::Used){
        set_state(fno,FrameState::Free);
        nFreeFrames++;
    }
}

/*--------------------------------------------------------------------------*/
/* DATA STRUCTURES */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* CONSTANTS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* FORWARDS */
/*--------------------------------------------------------------------------*/

/* -- (none) -- */

/*--------------------------------------------------------------------------*/
/* METHODS FOR CLASS   C o n t F r a m e P o o l */
/*--------------------------------------------------------------------------*/

ContFramePool * ContFramePool::head = NULL;
int ContFramePool::num_pools = 0;

ContFramePool::ContFramePool(unsigned long _base_frame_no,
                             unsigned long _n_frames,
                             unsigned long _info_frame_no)
{
    // TODO: IMPLEMENTATION NEEEDED!
    base_frame_no = _base_frame_no;
    nframes = _n_frames;
    nFreeFrames = _n_frames;
    info_frame_no = _info_frame_no;
    if(info_frame_no == 0) {
        bitmap = (unsigned char *) (base_frame_no * FRAME_SIZE);
    } else {
        bitmap = (unsigned char *) (info_frame_no * FRAME_SIZE);
    }

    for(int fno = 0; fno < nframes; fno++) {
        set_state(fno, FrameState::Free);
    }
    
    if(_info_frame_no == 0) {
        set_state(0, FrameState::HoS);
        nFreeFrames--;
    }
    
    if(head == NULL){
        head = this;
        next = NULL;
    }else{
        ContFramePool * cur = head;
        for(int i = 1; i < num_pools; i++){
            cur = cur->next;
        }
        cur->next = this;
    }
    num_pools++;

    // assert(false);
}

unsigned long ContFramePool::get_frames(unsigned int _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!

    if(nFreeFrames < _n_frames){
        return 0;
    }

    unsigned int frame_no = 0;
    unsigned int count = 0;
    unsigned int is_exist = 0;


    while(frame_no <= nframes && is_exist == 0){
        if(get_state(frame_no) != FrameState::Free){
            frame_no++;
        } else{
            if(get_state(frame_no+count) == FrameState::Free && count < _n_frames){
                count++;
            } else if (count == _n_frames){
                is_exist = 1;
            } else{
                frame_no += count;
                frame_no ++;
                count = 0;
            }
        }
    }

    if(is_exist == 1){
        set_state(frame_no, FrameState::HoS);
        nFreeFrames--;
        for(int i = 1; i < _n_frames; i++){
            set_state(frame_no+i, FrameState::Used);
            nFreeFrames--;
        }
        return (frame_no + base_frame_no);
    } else{
        return 0;
    }
    // Console::puts("ContframePool::get_frames not implemented!\n");
    // assert(false);
}

void ContFramePool::mark_inaccessible(unsigned long _base_frame_no,
                                      unsigned long _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
    set_state(_base_frame_no - base_frame_no, FrameState::HoS);
    nFreeFrames--;
    for(int i = 1; i < _n_frames; i++){
        set_state(_base_frame_no - base_frame_no + i, FrameState::Used);
        nFreeFrames--;
    }
    // Console::puts("ContframePool::mark_inaccessible not implemented!\n");
    // assert(false);
}

void ContFramePool::release_frames(unsigned long _first_frame_no)
{
    // TODO: IMPLEMENTATION NEEEDED!
    
    ContFramePool * release_pool = head;
    while(_first_frame_no > (release_pool->base_frame_no + release_pool->nframes) || _first_frame_no < release_pool->base_frame_no){
        release_pool = release_pool->next;
    }

    release_pool->release_frame_in_pool(_first_frame_no);

    // Console::puts("ContframePool::release_frames not implemented!\n");
    // assert(false);
}

unsigned long ContFramePool::needed_info_frames(unsigned long _n_frames)
{
    // TODO: IMPLEMENTATION NEEEDED!
    return _n_frames / 16384 + (_n_frames % 16384 > 0 ? 1 : 0);
    // Console::puts("ContframePool::need_info_frames not implemented!\n");
    // assert(false);
}

void ContFramePool::get_info_of_linked_list()
{
    Console::puts("The frames of linked list \n");
    ContFramePool * cur = head;
    for(int i = 0; i < num_pools; i++){
        Console::puts("The number of frames in ");
        Console::puti(i+1);
        Console::puts(" pool: ");
        Console::puti(cur->nframes);
        Console::puts("\n");
        cur = cur -> next;
    }
}
