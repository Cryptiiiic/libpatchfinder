//
//  ibootpatchfinder64_iOS14.cpp
//  liboffsetfinder64
//
//  Created by tihmstar on 28.07.20.
//  Copyright © 2020 tihmstar. All rights reserved.
//

#include <libgeneral/macros.h>
#include "ibootpatchfinder64_iOS14.hpp"

using namespace std;
using namespace tihmstar::offsetfinder64;
using namespace tihmstar::libinsn;

#define iBOOT_BASE_OFFSET 0x300
#define PROD "effective-production-status-ap"

ibootpatchfinder64_iOS14::ibootpatchfinder64_iOS14(const char *filename)
    : ibootpatchfinder64_base(filename)
{
    _entrypoint = _base = (loc_t)*(uint64_t*)&_buf[iBOOT_BASE_OFFSET];
    _vmem = new vmem({{_buf,_bufSize,_base, vsegment::vmprot::kVMPROTREAD | vsegment::vmprot::kVMPROTWRITE | vsegment::vmprot::kVMPROTEXEC}});
    debug("iBoot base at=0x%016llx\n", _base);
}

ibootpatchfinder64_iOS14::ibootpatchfinder64_iOS14(const void *buffer, size_t bufSize, bool takeOwnership)
    : ibootpatchfinder64_base(buffer,bufSize,takeOwnership)
{
    _entrypoint = _base = (loc_t)*(uint64_t*)&_buf[iBOOT_BASE_OFFSET];
    _vmem = new vmem({{_buf,_bufSize,_base, vsegment::vmprot::kVMPROTREAD | vsegment::vmprot::kVMPROTWRITE | vsegment::vmprot::kVMPROTEXEC}});
    debug("iBoot base at=0x%016llx\n", _base);
}

std::vector<patch> ibootpatchfinder64_iOS14::get_sigcheck_patch(){
    std::vector<patch> patches;
    loc_t img4decodemanifestexists;
    if(_vers >= 7459 && _vers < 8419) {
            img4decodemanifestexists = _vmem->memmem("\xE8\x03\x00\xAA\xC0\x00\x80\x52\x28\x01\x00\xB4", 12);
    } else {
        img4decodemanifestexists = _vmem->memmem("\xE8\x03\x00\xAA\xC0\x00\x80\x52\xE8\x00\x00\xB4", 12); //0x180032144;
    }
    debug("img4decodemanifestexists=%p",img4decodemanifestexists);
    retassure(img4decodemanifestexists, "retassure: %d", __LINE__);

    loc_t img4decodemanifestexistsref = find_call_ref(img4decodemanifestexists);
    debug("img4decodemanifestexistsref=%p",img4decodemanifestexistsref);
    retassure(img4decodemanifestexistsref, "retassure: %d", __LINE__);

    vmem iter(*_vmem,img4decodemanifestexistsref);
    vmem iter2(*_vmem,img4decodemanifestexistsref);

    while(++iter != insn::adr) continue;
    if(_vers >= 10151 && _vers_arr[0] > 80) {
      while(++iter != insn::adr) continue;
    }
    if((uint8_t)iter().rd() != 2) {
        while(++iter2 != insn::adr);
        retassure((uint8_t)iter().rd() == 2, "retassure: %d", __LINE__);
    }
    loc_t img4interposercallbackptr = iter().imm();
    debug("img4interposercallbackptr=%p",img4interposercallbackptr);
    retassure(img4interposercallbackptr, "retassure: %d", __LINE__);

    loc_t img4interposercallback = _vmem->deref(img4interposercallbackptr);
    debug("img4interposercallback=%p",img4interposercallback);
    retassure(img4interposercallback, "retassure: %d", __LINE__);

    vmem iter3(*_vmem,img4interposercallback);
    while(++iter3 != insn::ret);
    loc_t img4interposercallbackret = iter3().pc();
    retassure(img4interposercallbackret, "retassure: %d", __LINE__);
    debug("img4interposercallbackret=%p",img4interposercallbackret);
    if(--iter3 == insn::add) {
        while(--iter3 == insn::ldp);
        if(iter3() != insn::mov) {
            while(--iter3 != insn::nop);
        }
        loc_t img4interposercallbackmov = iter3().pc();
        retassure(img4interposercallbackmov, "retassure: %d", __LINE__);
        debug("img4interposercallbackmov=%p",img4interposercallbackmov);
        patches.push_back({img4interposercallbackmov, "\x00\x00\x80\xD2" /*mov x0, 0*/, 4});
        while (++iter3 != insn::ret);
        while (++iter3 != insn::ret);
        loc_t img4interposercallbackret2 = iter3().pc();
        retassure(img4interposercallbackret2, "retassure: %d", __LINE__);
        debug("img4interposercallbackret2=%p", img4interposercallbackret2);
        patches.push_back({img4interposercallbackret2 - 4, "\x00\x00\x80\xD2" /*mov x0, 0*/, 4});
    } else {
        patches.push_back({img4interposercallbackret - 4, "\x00\x00\x80\xD2" /*mov x0, 0*/, 4});
        int i = 0;
        for(; i < 100; i++) {
          while(--iter3 != insn::b) continue;
          if(--iter3 == insn::ldp) {
            break;
          }
        }
        if(i == 99) {
            reterror("img4interposercallback couldn't find branch for ret2!");
        }
        while(--iter3 != insn::mov) continue;
        loc_t img4interposercallbackmovx20 = iter3().pc();
        debug("img4interposercallbackmovx20=%p", img4interposercallbackmovx20);
        patches.push_back({img4interposercallbackmovx20, "\x00\x00\x80\xD2" /*mov x0, 0*/, 4});
    }
    return patches;
}

std::vector<patch> ibootpatchfinder64_iOS14::get_demotion_patch(){
    std::vector<patch> patches;
    /* always production patch*/
    debug("prod search");
    if(!_vmem->memstr(PROD))
        return patches;
    loc_t productionStr = _vmem->memstr(PROD);
    debug("prod done");
    debug("productionStr=%p\n",productionStr);
    assure(productionStr);
    loc_t productionRef = find_literal_ref(productionStr);
    debug("productionRef=%p\n",productionRef);
    assure(productionRef);
    vmem iter(*_vmem,productionRef);
    while (++iter != insn::bl);
    ++iter;
    while (++iter != insn::bl);
    iter = iter().imm();
    while (++iter != insn::bl);
    loc_t demoteRef = iter().imm();
    if (demoteRef) {
        iter = demoteRef;
        while (++iter != insn::b);
        iter = iter().imm();
        assure((uint32_t)iter().imm() == 1 || (uint32_t)iter().imm() == 0x100);
        demoteRef = iter;
        debug("demoteRef=%p\n",demoteRef);
        patches.push_back({demoteRef,"\x20\x00\x80\xD2" /*mov x0, 0*/,4});
        if(++iter != insn::ret) {
            demoteRef = iter;
            debug("demoteRef2=%p\n",demoteRef);
            patches.push_back({demoteRef,"\xD5\x03\x20\x1F" /*nop*/,4});
        }
    }
    return patches;
}

std::vector<patch> ibootpatchfinder64_iOS14::get_change_reboot_to_fsboot_patch(){
    std::vector<patch> patches;

    loc_t rebootstr = findstr("reboot", true);
    debug("rebootstr=%p",rebootstr);

    loc_t rebootrefstr = _vmem->memmem(&rebootstr,sizeof(loc_t));
    debug("rebootrefstr=%p",rebootrefstr);
    
    loc_t rebootrefptr = rebootrefstr+8;
    debug("rebootrefptr=%p",rebootrefptr);
    
    loc_t fsbootstr = findstr("fsboot", true);
    debug("fsbootstr=%p",fsbootstr);

    patches.push_back({rebootrefstr,&fsbootstr,sizeof(loc_t)}); //rewrite pointer to point to fsboot

    loc_t fsbootrefstr = _vmem->memmem(&fsbootstr,sizeof(loc_t));
    debug("fsbootrefstr=%p",fsbootrefstr);
    
    loc_t fsbootfunction = _vmem->deref(fsbootrefstr+8);
    debug("fsbootfunction=%p",fsbootfunction);
    patches.push_back({rebootrefstr+8,&fsbootfunction,sizeof(loc_t)}); //rewrite pointer to point to fsboot

    return patches;
}

loc_t ibootpatchfinder64_iOS14::find_iBoot_logstr(uint64_t loghex, int skip, uint64_t shortdec){
    vmem iter(*_vmem);
    uint64_t longval = 0;
    uint64_t shortval = 0;

    while (true) {
        while (++iter != insn::movz || iter().rd() != 9);
        longval = iter().imm();

        {
            vmem prevIter{iter,iter.pc()-4};
            if (prevIter() == insn::movz && prevIter().rd() == 8) {
                shortval = prevIter().imm();
            }
        }
        while (++iter == insn::movk && iter().rd() == 9){

            uint64_t curval =  iter().imm();
            
            longval += curval;
        }
        if (longval == loghex && (shortdec == shortval || shortdec == 0)){
            if (skip-- == 0) return iter;
        }
    }
    
    return 0;
}


uint32_t ibootpatchfinder64_iOS14::get_el1_pagesize(){
    vmem iter(*_vmem);

    while (++iter != insn::msr || iter().special() != insn::tcr_el1);
    
    loc_t write_tcr_el1 = iter;
    debug("write_tcr_el1=%p",write_tcr_el1);

    loc_t ref_write_tcr_el1 = find_call_ref(write_tcr_el1);
    debug("ref_write_tcr_el1=%p",ref_write_tcr_el1);
    
    iter = ref_write_tcr_el1;
    --iter;
    
    assure(iter() == insn::bl);
    
    loc_t get_tcr_el1 = iter().imm();
    debug("get_tcr_el1=%p",get_tcr_el1);
    
    iter = get_tcr_el1;
    
    while (++iter != insn::ret);
    loc_t get_tcr_el1_eof = iter;
    debug("get_tcr_el1_eof=%p",get_tcr_el1_eof);
    
    uint64_t tcr_el1_val = find_register_value(get_tcr_el1_eof,0,get_tcr_el1);
    debug("tcr_el1_val=%p",tcr_el1_val);

    uint8_t TG0 = (tcr_el1_val >> 14) & 0b11;
    
    switch (TG0) {
        case 0b00:
            return 0x1000;
        case 0b01:
            return 0x10000;
        case 0b10:
            return 0x4000;
        default:
            reterror("invalid TG0=%d",TG0);
    }
}


std::vector<patch> ibootpatchfinder64_iOS14::get_rw_and_x_mappings_patch_el1(){
    std::vector<patch> patches;

    uint32_t pagesize = get_el1_pagesize();
    
    vmem iter(*_vmem);

    while (++iter != insn::msr || iter().special() != insn::ttbr0_el1);
    
    loc_t write_ttbr0_el1 = iter;
    debug("write_ttbr0_el1=%p",write_ttbr0_el1);
    
    while (++iter != insn::ret);
    loc_t write_ttbr0_el1_eof = iter;
    debug("write_ttbr0_el1_eof=%p",write_ttbr0_el1_eof);

    uint32_t pageshift = pageshit_for_pagesize(pagesize);

    /*
     create iboot_base_block_entry (rw)
     */
    uint64_t iboot_base_block_entry = find_base();

    //mask off unneeded bits
    iboot_base_block_entry = pte_vma_to_index(pagesize,2,iboot_base_block_entry);
    iboot_base_block_entry = pte_index_to_vma(pagesize, 2, iboot_base_block_entry);
    
    //set pte bits
    iboot_base_block_entry |= 0x445;
    
    /*
     create loadaddr_block_entry (rx)
     */
    loc_t loadaddr_str = findstr("loadaddr", true);
    debug("loadaddr_str=%p",loadaddr_str);
    loc_t loadaddr = 0;
    while (!loadaddr) {
        loc_t loadaddr_ref = find_literal_ref(loadaddr_str);
        debug("loadaddr_ref=%p",loadaddr_ref);
        vmem iter(*_vmem,loadaddr_ref);

        while (++iter != insn::bl);
        loc_t loadaddr_ref_firstbl = iter;
        debug("loadaddr_ref_firstbl=%p",loadaddr_ref_firstbl);

        loadaddr = find_register_value(loadaddr_ref_firstbl, 1, loadaddr_ref);
    }
    
    debug("loadaddr=%p",loadaddr);

    //mask off unneeded bits
    uint64_t loadaddr_block_entry = pte_vma_to_index(pagesize,2,loadaddr);
    loadaddr_block_entry = pte_index_to_vma(pagesize, 2, loadaddr_block_entry);

    //set pte bits
    loadaddr_block_entry |= 0x4c5;
    
    uint32_t orig_write_ttbr0_el1_insn_cnt = (write_ttbr0_el1_eof-write_ttbr0_el1+4)/4;
    bool needs_alignment = ! (orig_write_ttbr0_el1_insn_cnt&1);
    

    /*
     create patch
              ldr        x1, =0x180000445
              nop
              ldr        x2, =0x8000004c5
              stp        x1, x2, [x0, #0x8]
              dmb        sy
        ///orig
              msr        ttbr0_el1, x0
              isb
              ret
        ///orig end
              dq         0x0000000180000445
              dq         0x00000008000004c5
    */
    uint8_t cinsn = 0;
    {
        //ldr        x1, =0x180000445
        insn pins = insn::new_literal_ldr(0, (5+orig_write_ttbr0_el1_insn_cnt)*4, 1);
        uint32_t opcode = pins.opcode();
        patches.push_back({(loc_t)(cinsn++*4),&opcode, sizeof(opcode)});
    }
    {
        //nop
        insn pins = insn::new_general_nop(0);
        uint32_t opcode = pins.opcode();
        patches.push_back({(loc_t)(cinsn++*4),&opcode, sizeof(opcode)});
    }
    {
        //ldr        x2, =0x8000004c5
        insn pins = insn::new_literal_ldr(0, (5+orig_write_ttbr0_el1_insn_cnt)*4, 2);
        uint32_t opcode = pins.opcode();
        patches.push_back({(loc_t)(cinsn++*4),&opcode, sizeof(opcode)});
    }
    {
        //stp        x1, x2, [x0, #0x8]
        insn pins = insn::new_general_stp_offset(0, 8, 1, 2, 0);
        uint32_t opcode = pins.opcode();
        patches.push_back({(loc_t)(cinsn++*4),&opcode, sizeof(opcode)});
    }
    //dmb        sy
    patches.push_back({(loc_t)(cinsn++*4),"\xBF\x3F\x03\xD5",4});
    
    uint32_t *orig_opcodes = (uint32_t*)memoryForLoc(write_ttbr0_el1);
    patches.push_back({(loc_t)(cinsn*4),orig_opcodes,4*orig_write_ttbr0_el1_insn_cnt});cinsn+=orig_write_ttbr0_el1_insn_cnt;
    
    patches.push_back({(loc_t)(cinsn*4),&iboot_base_block_entry,sizeof(iboot_base_block_entry)}); cinsn+=sizeof(iboot_base_block_entry)/4;
    patches.push_back({(loc_t)(cinsn*4),&loadaddr_block_entry,sizeof(loadaddr_block_entry)}); cinsn+=sizeof(loadaddr_block_entry)/4;

    uint32_t fullpatch_size = 0;
    for (auto p: patches){
        fullpatch_size += p._patchSize;
    }
    
    /*
     now find an empty spot for to place the payload
     */
    loc_t nopspace = findnops(fullpatch_size/4 + 1, true); //alloc 1 more nop than we need
    debug("nopspace=%p",nopspace);
    
    /*
     ldp needs to load from 8 byte aligned address
     if needs_alignment is false, we start at 8 byte aligned address and the data is placed at 8 byte aligned
     if needs_alignmend is true, then we need to start at 8byte aligned + 4 for the data to be 8 byte aligned
     */
    if (((nopspace & 0x4) && !needs_alignment /* we don't need alignment, but we start at an +4 address*/ )
        || (((nopspace & 0x4) == 0) && needs_alignment) /* we are at 8 byte aligned, but we need fixup*/) {
        nopspace+=4;
    }
    
    //now fixup payload addresses
    for (auto &p: patches){
        p._location += nopspace;
    }
    
    /*
     finally rewrite write_ttbr0 to jump to payload
     */
    for (loc_t iloc = write_ttbr0_el1; iloc<write_ttbr0_el1_eof; iloc+=4) {
        insn pins = insn::new_general_nop(iloc);
        uint32_t opcode = pins.opcode();
        patches.push_back({pins,&opcode,sizeof(opcode)});
    }
    
    {
        //make last orig insn of write_ttbr0 jump to payload
        insn pins = insn::new_immediate_b(write_ttbr0_el1_eof, nopspace);
        uint32_t opcode = pins.opcode();
        patches.push_back({pins,&opcode,sizeof(opcode)});
    }
    
    
    return patches;
}
