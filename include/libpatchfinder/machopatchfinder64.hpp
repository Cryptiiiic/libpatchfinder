//
//  machopatchfinder64.hpp
//  libpatchfinder
//
//  Created by tihmstar on 28.09.19.
//  Copyright © 2019 tihmstar. All rights reserved.
//

#ifndef machopatchfinder64_hpp
#define machopatchfinder64_hpp

#include <libpatchfinder/patchfinder64.hpp>
#include <unordered_map>

struct symtab_command;
namespace tihmstar {
    namespace patchfinder {

        class machopatchfinder64 : public patchfinder64{
            std::vector<std::pair<const struct symtab_command *,uint8_t *>> __symtabs;
            std::unordered_map<uint64_t, uint64_t> _fileoff_map;
            std::pair<uint8_t *, uint32_t> _fat;

            std::vector<libinsn::vsegment> loadSegmentsForMachHeader(void *mh);
            void loadSegments();
            __attribute__((always_inline)) const std::vector<std::pair<const struct symtab_command *,uint8_t *>> &getSymtabs();

            void init();

        public:
            machopatchfinder64(const char *filename);
            machopatchfinder64(const void *buffer, size_t bufSize, bool takeOwnership = false);

            machopatchfinder64(const machopatchfinder64 &cpy) = delete; //delete copy constructor
            machopatchfinder64(machopatchfinder64 &&mv); //move constructor

            bool haveSymbols() { return __symtabs.size();};
            loc_t find_sym(const char *sym);
            std::string sym_for_addr(loc_t addr);
            loc_t bl_jump_stub_ptr_loc(loc_t bl_insn);
            std::unordered_map<uint64_t, uint64_t> get_fileoff_map() { return this->_fileoff_map; };
            std::pair<uint8_t *, uint32_t> get_fat() { return this->_fat; };
            const tihmstar::libinsn::vmem<libinsn::arm64::insn> *get_vmem() { return this->_vmem; };

        };

    };
};


#endif /* machopatchfinder64_hpp */
