/* Subroutines used for Microchip PIC32C code generation.
   Copyright (C) 1989, 1990, 1991, 1993, 1994, 1995, 1996, 1997, 1998,
   1999, 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007
   Free Software Foundation, Inc.
   Contributed by Rathish C, rathish.chamukuttan@microchip.com

This file is part of GCC.

GCC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3, or (at your option)
any later version.

GCC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GCC; see the file COPYING3.  If not see
<http://www.gnu.org/licenses/>.  */


#ifndef __PIC32C_PROTOS_H
#define __PIC32C_PROTOS_H


/* Function Prototypes*/

extern void pic32c_subtarget_override_options(void);

extern section *mchp_select_section
                  (tree decl, int reloc, unsigned HOST_WIDE_INT align ATTRIBUTE_UNUSED);

extern void mchp_write_encoded_name(FILE *, const char *);

// TODO: May not need to be exposed.
extern const char *mchp_strip_name_encoding 
                  (const char *symbol_name);

extern void mchp_asm_named_section
                  (const char *pszSectionName, SECTION_FLAGS_INT flags, tree decl ATTRIBUTE_UNUSED);

extern void mchp_subtarget_encode_section_info 
                  (tree decl, rtx rtl, int first_seen ATTRIBUTE_UNUSED);

extern SECTION_FLAGS_INT mchp_section_type_flags
                   (tree decl, const char *name, int reloc ATTRIBUTE_UNUSED);

extern void pic32c_subtarget_encode_section_info
                  (tree decl, rtx rtl, int first_seen ATTRIBUTE_UNUSED);

extern tree mchp_get_smartio_fndecl (int);

extern const char * mchp_text_section_asm_op(void);
extern const char * mchp_data_section_asm_op(void);
extern const char * mchp_bss_section_asm_op(void);
extern const char * mchp_sbss_section_asm_op(void);
extern const char * mchp_sdata_section_asm_op(void);
extern const char * mchp_rdata_section_asm_op(void);

extern void pic32c_file_end (void);


/* Attribute processing */
extern tree pic32c_address_attribute (tree *, tree, tree , int , bool *);

extern tree pic32c_unsupported_attribute (tree *, tree, tree , int , bool *);

extern tree pic32c_space_attribute (tree *decl, tree identifier, tree args,
                int flags ATTRIBUTE_UNUSED, bool *no_add_attrs);

extern tree pic32c_tcm_attribute (tree *, tree, tree , int , bool *);

extern tree pic32c_noload_attribute (tree *, tree, tree , int , bool *);

extern bool pic32c_attribute_takes_identifier_p (const_tree attr_id);

extern tree pic32c_keep_attribute(tree *decl, tree identifier ATTRIBUTE_UNUSED,
                                tree args, int flags ATTRIBUTE_UNUSED,
                                bool *no_add_attrs);

extern tree pic32c_unique_section_attribute (tree *node, tree name ATTRIBUTE_UNUSED,
                          tree args ATTRIBUTE_UNUSED, int flags ATTRIBUTE_UNUSED,
                          bool *no_add_attrs);

/* Builtin functions */
extern void pic32c_subtarget_init_builtins (void);
extern rtx pic32c_subtarget_expand_builtin (unsigned int code,
                                            tree exp, rtx target);
extern tree pic32c_subtarget_builtin_decl (unsigned code, bool initialize_p); 
extern bool pic32c_subtarget_gimple_fold_builtin (gimple_stmt_iterator*);

/* pragmas */
extern void mchp_handle_config_pragma(struct cpp_reader *);

#endif /* __PIC32C_PROTOS_H */

