#ifndef __PIC32_DINIT__
#define __PIC32_DINIT__

#ifndef PIC32
#define PIC32
#endif

#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "libiberty.h"
#include "elf-bfd.h"

typedef enum
{
    DINIT_CLEAR = 0,
    DINIT_COPY,         /* note matching definition in crt0.c */
    DINIT_COPY_VAL_EMB,
    DINIT_COPY_VAL_DATA,
    DINIT_COMPRESSED
} dinit_type;

extern bfd_boolean  pic32_data_init;
extern bfd_boolean  pic32_has_data_init_option;
extern bfd_boolean  pic32_has_fill_option;
extern bfd_boolean  pic32_has_stack_option;
extern bfd_boolean  pic32_has_processor_option;
extern bfd_boolean  pic32_has_hardfloat_option;
extern bfd_boolean  pic32_has_softfloat_option;
extern bfd_boolean  pic32_code_in_dinit;
extern bfd_boolean  pic32_has_code_in_dinit_option;
extern bfd_boolean  pic32_dinit_in_serial_mem;
extern bfd_boolean  pic32_has_dinit_in_serial_mem_option;
extern bfd_boolean  pic32_dinit_has_absolute_address;
extern bfd_vma      dinit_address;
extern bfd_boolean  pic32_debug;

/* Set legacy as default */
unsigned int dinit_compress_level = 0;

/* Data Structures for the Data Init Template */
bfd             *init_bfd;
unsigned char   *init_data = NULL;
asection        *init_template = 0;
static uint32_t saved_dinit;


/* Data Structures for Input Data Sections */
extern struct pic32_section *data_sections;

/* Data Structures for Input Data Sections */
extern struct pic32_section *code_sections;

static void pic32_add_section_to_list
  (struct pic32_section *lst, asection *sec);

static void bfd_pic32_process_data_section
  (asection * const sec, PTR fp, bfd_boolean do_set);
static void bfd_pic32_process_code_section
  (struct pic32_section * const s, PTR fp, bfd_boolean update_flags, bfd_boolean do_set);
static void bfd_pic32_write_data_header
  (unsigned char **,  bfd_vma,  bfd_vma, uint16_t, bfd_boolean);
static unsigned int dinit_compress
  (unsigned char * const buf, unsigned int entry_len, PTR fp, bfd_boolean do_set);
bfd_boolean is_compressable
  (PTR fp, unsigned int length);
static unsigned char *dinit_get_section_contents
  (asection *, bfd_boolean);

/* Gets content from asection's input or output section depending on do_set */
static unsigned char *
dinit_get_section_contents(asection *sect, bfd_boolean do_set)
{
  unsigned char * buf;

  /*
  ** Note: We are extracting input section data
  ** from an output section.
  */

  if (do_set)
    buf = (unsigned char *)malloc(sect->output_section->size);
  else
    buf = (unsigned char *)malloc(sect->size);

  if (!buf)
  {
    fprintf(stderr, "Link Error: not enough memory for section contents\n");
    abort();
  }

  /* Reset the bfd file pointer, because
        there seems to be a bug with fseek()
        in Winblows that makes seeking to
        a position earlier in the file unreliable. */

  bfd_boolean got_contents = FALSE;

  if (do_set)
  {
    bfd_seek(sect->output_section->owner, 0, SEEK_SET);
    got_contents = !bfd_get_section_contents(sect->output_section->owner,
                                             sect->output_section, buf,
                                             sect->output_offset, sect->size);
  }
  else
  {
    bfd_seek(sect->owner, 0, SEEK_SET);
    got_contents = !bfd_get_section_contents(sect->owner, sect, buf, 0,
                                             sect->size);
  }

  if (got_contents)
  {
    fprintf(stderr, "Link Error: can't load section %s contents\n", sect->name);
    abort();
  }

  return buf;
}

static void
pic32_add_section_to_list (struct pic32_section * lst,
                              asection *sec)
{
  struct pic32_section *s, *tail, *new;

  if (!(lst))
    return;
  /* create a new element */
  new = ((struct pic32_section *)
         xmalloc(sizeof(struct pic32_section)));
  new->next  = 0;
  new->sec  = sec;
  new->attributes = 0 /*pic32_attribute_map(sec) */;
  new->file = NULL; /* not set at this point */

  /* find the tail */
  for (s = lst; s != NULL; s = s->next)
    tail = s;

  tail->next = new;  /* add it */
}


/*
 * ** Write a data record header
 * */
static void
bfd_pic32_write_data_header(d, addr, len, format, do_set)
     unsigned char **d;
     bfd_vma addr;
     bfd_vma len;
     uint16_t format;
	 bfd_boolean do_set;
{
  if (do_set)
  {
    addr &= 0xFFFFFFFF;

    /* write destination address */
    *(*d)++ = (unsigned char) (addr & 0xFF);
    *(*d)++ = (unsigned char) ((addr >> 8) & 0xFF);
    *(*d)++ = (unsigned char) ((addr >> 16) & 0xFF);
    *(*d)++ = (unsigned char) ((addr >> 24) & 0xFF);

    /* write destination length */
    *(*d)++ = (unsigned char) (len & 0xFF);
    *(*d)++ = (unsigned char) ((len >> 8) & 0xFF);
    *(*d)++ = (unsigned char) ((len >> 16) & 0xFF);
    *(*d)++ = (unsigned char) ((len >> 24) & 0xFF);

    /* write format code */
    *(*d)++ = (unsigned char) (format & 0xFF);
    *(*d)++ = (unsigned char) ((format >> 8) & 0xFF);
    *(*d)++ = 0;
    *(*d)++ = 0;
  }
}

static bfd_boolean dinit_is_relocatable(asection * const sec)
{
  if (sec->reloc_count || sec->was_reloc || sec->flags & SEC_RELOC)
    return TRUE;
  return FALSE;
}

static bfd_boolean dinit_is_bss_section(asection * const sec)
{
  if ((PIC32_IS_BSS_ATTR(sec) || PIC32_IS_BSS_ATTR_WITH_MEMORY_ATTR(sec))
      && (sec->size > 0)
      && !dinit_is_relocatable(sec))
    return TRUE;
  return FALSE;
}

static bfd_boolean dinit_is_data_section (asection * const sec)
{
  if ((PIC32_IS_DATA_ATTR(sec) ||
        PIC32_IS_DATA_ATTR_WITH_MEMORY_ATTR(sec) ||
        PIC32_IS_RAMFUNC_ATTR(sec)) &&
        (sec->size > 0))
    return TRUE;
  return FALSE;
}

static bfd_boolean dinit_is_valid_section (asection * const sec)
{
  /* skip persistent or noload data sections*/
  if (PIC32_IS_PERSIST_ATTR(sec) || PIC32_IS_NOLOAD_ATTR(sec) || (sec->size == 0))
    {
      if (pic32_debug)
        printf("  %s (skipped), size = %x\n",
               sec->name, (unsigned int) sec->size);
      return FALSE;
    }
  return TRUE;
}

static void dinit_reset_section_flags(asection * const sect, bfd_boolean do_set)
{
  if (do_set)
  {
    Elf_Internal_Shdr *this_hdr;

    /* make section not LOADable */
    sect->output_section->flags &= ~SEC_LOAD;
    sect->output_section->flags |= SEC_NEVER_LOAD;

    sect->flags &= ~SEC_LOAD;
    sect->flags |= SEC_NEVER_LOAD;

    this_hdr = &elf_section_data(sect->output_section)->this_hdr;
    this_hdr->sh_flags |= SHF_NOLOAD;
  }
}

/*
** If the section is BSS or DATA, write the appropriate
** record into the data init template.
**
** ToDo: If the data section exceeds 64K, break into
** multiple records.
*/
static void bfd_pic32_process_data_section(asection * const sect, PTR fp, bfd_boolean do_set)
{
  unsigned char *buf,*p;
  unsigned char **d = (unsigned char **) fp;
  bfd_vma runtime_size = sect->size;
  bfd_vma runtime_addr;
  Elf_Internal_Shdr *this_hdr;


    if (sect->output_section == NULL)
        return;

#ifdef TARGET_IS_PIC32C
    if (!sect->init)
    {
      if (pic32_debug)
	    fprintf (stderr, "data_section %s (skipped, !init)\n", sect->name);
      return;
    }
#endif

    if (sect->size > 0)
        runtime_addr = sect->output_offset + sect->output_section->vma;

  /* skip persistent or noload data sections */
  if (!dinit_is_valid_section(sect))
    return;

  /* process BSS-type sections */
  if (dinit_is_bss_section(sect))
    {
      if (pic32_debug)
        printf("  %s (bss), size = %x bytes, template addr = %lx\n",
               sect->name, (unsigned int) runtime_size,
               (long unsigned int) *d);

      /* write header only */
      bfd_pic32_write_data_header(d, runtime_addr, runtime_size, DINIT_CLEAR, do_set);
    }

   /* process DATA-type sections */
  if (dinit_is_data_section(sect))
    {
      if (pic32_debug)
        printf("  %s (data), size = %x bytes, template addr = %lx\n",
               sect->name, (unsigned int) runtime_size,
               (long unsigned int) *d);

      buf = dinit_get_section_contents(sect, do_set);

      int count = 0;
      /* write header */
      bfd_pic32_write_data_header(d, runtime_addr, runtime_size, DINIT_COPY, do_set);
      if (do_set)
        {
          for (p = buf; p < (buf + sect->size); )
            {
              *(*d)++ = *p++;
            }
          /* fill template with zeroes
           * padding for word alignment
           */
          count = sect->size % 4;
          if (count) while (count < 4) { *(*d)++ = 0; count++; }
        }

      free(buf);

      /* make section not LOADable */
      dinit_reset_section_flags(sect, do_set);
    } /* process DATA-type sections */

  return;
} /* static void bfd_pic32_process_data_section (...)*/


static void bfd_pic32_process_code_section(struct pic32_section * const s, PTR fp, bfd_boolean update_flags, bfd_boolean do_set)
{
  unsigned char *buf,*p;
  unsigned char **d = (unsigned char **) fp;
  asection* sect = s->sec;
  bfd_vma runtime_size = sect->size;
  bfd_vma runtime_addr = sect->output_offset + sect->output_section->vma;
  Elf_Internal_Shdr *this_hdr;


#ifdef TARGET_IS_PIC32C
    ///\ set runtime address for .vectors
    ///\ itcm origin
    if(!update_flags)
        runtime_addr = pic32c_itcm_vectors_address;
#endif

  if (PIC32_IS_CODE_ATTR(sect) && (sect->size > 0)
#ifdef TARGET_IS_PIC32C
        && sect->init
#endif
  )
    {
      if (pic32_debug)
        printf("  %s (data), size = %x bytes, template addr = %lx\n",
               sect->name, (unsigned int) runtime_size,
               (long unsigned int) *d);

      buf = dinit_get_section_contents(sect, do_set);

      { int count = 0;
          /* write header */
          bfd_pic32_write_data_header(d, runtime_addr, runtime_size, DINIT_COPY, do_set);
          if (do_set)
            {
              for (p = buf; p < (buf + sect->size); )
                {
                  *(*d)++ = *p++;
                  count ++;
                  if (count == 4)
                    count = 0;
                }
              /* fill template with zeroes */
              if (count) while (count < 4) { *(*d)++ = 0; count++; }
            }
        }

        free(buf);
#ifdef TARGET_IS_PIC32C
        if (update_flags)
#endif
          /* make section not LOADable */
          dinit_reset_section_flags(sect, do_set);
    }
}

/* check if 2 sections are compatible to be merged at dinit level */
static bfd_boolean are_dinit_compatible(asection * const s1, asection * const s2)
{
  bfd_vma s1_addr = s1->output_offset + s1->output_section->vma;
  bfd_vma s2_addr = s2->output_offset + s2->output_section->vma;


  if (s1 && s2
      /* same type */
      && ( (dinit_is_bss_section(s1) && dinit_is_bss_section(s2))
          || (dinit_is_data_section(s1) && dinit_is_data_section(s2)))
      /* continuous addresses */
      && ((s1_addr + s1->size) == s2_addr)
      /* Not relocatable sections */
      && !(dinit_is_relocatable(s1) && dinit_is_relocatable(s2)))
    return TRUE;
  return FALSE;
}

static void dinit_gen_repeated_value(PTR* fp, bfd_vma runtime_addr, bfd_vma runtime_size,
                            uint32_t value, dinit_type data_type)
{
    unsigned char **d = (unsigned char**)fp;
    switch (data_type)
    {
        case DINIT_COPY_VAL_EMB:
            bfd_pic32_write_data_header(d, runtime_addr, runtime_size, DINIT_COPY_VAL_EMB, TRUE);
            *d -= 2; /* Utilize zeros placed by write_data_header */
            *(*d)++ = (unsigned char)(value & 0xFF);
            *(*d)++ = (unsigned char)((value >> 8) & 0xFF);
            break;

        case DINIT_COPY_VAL_DATA:
            bfd_pic32_write_data_header(d, runtime_addr, runtime_size, DINIT_COPY_VAL_DATA, TRUE);
            *(*d)++ = (unsigned char)(value & 0xFF);
            *(*d)++ = (unsigned char)((value >> 8) & 0xFF);
            *(*d)++ = (unsigned char)((value >> 16) & 0xFF);
            *(*d)++ = (unsigned char)((value >> 24) & 0xFF);
            break;

        default:
            break;
    }
}


static dinit_type dinit_get_entry_type(asection* sect, PTR* fp, bfd_boolean do_set)
{
    unsigned char*  buf = NULL;
    uint32_t        repeated_int;
    uint16_t        repeated_short;
    bfd_size_type   use_copy_val_int = 0;
    bfd_size_type   use_copy_val_short = 0;
    bfd_vma         runtime_addr = sect->output_offset + sect->output_section->vma;
    unsigned char** d = (unsigned char**)fp;
    char*           p;

    /* reloc sections are always DINIT_COPY */
    if (dinit_is_relocatable(sect))
    {
      return DINIT_COPY;
    }

    buf = dinit_get_section_contents(sect, do_set);

	repeated_int = *(uint32_t*)buf;
	repeated_short = *(uint16_t*)buf;
	bfd_boolean check_int = !(sect->size % 4) && sect->size > 4;
	bfd_boolean check_short = !(sect->size % 2)  && sect->size > 2;

	/* Check for repeated values */
	for (p = buf; p < (buf + sect->size); p += 2)
	  {
		if (check_int && (uintptr_t) p % 4 == 0)
		  if (repeated_int == *(uint32_t *)p)
			use_copy_val_int++;

		if (check_short && repeated_short == *(uint16_t*)p)
		  use_copy_val_short++;
	  }

    free(buf);

	/* Ensure we only create a COPY_VAL_DATA/EMB type when the entire
        section is the same repeated value */
    if (use_copy_val_short && use_copy_val_short == sect->size/2)
    {
      saved_dinit += sect->size;
      if (do_set)
        dinit_gen_repeated_value(d, runtime_addr, sect->size, (uint32_t)repeated_short, DINIT_COPY_VAL_EMB);
      return DINIT_COPY_VAL_EMB;
    }

    if (use_copy_val_int && use_copy_val_int == sect->size/4)
    {
      saved_dinit += sect->size - 4;
      if (do_set)
        dinit_gen_repeated_value(d, runtime_addr, sect->size, repeated_int, DINIT_COPY_VAL_DATA);
      return DINIT_COPY_VAL_DATA;
    }
    /* dinit is checked just for data, it can't be DINIT_CLEAR */
    return DINIT_COPY;
}

struct pic32_section* const
bfd_pic32_process_merged_sections(struct pic32_section * const p32_start_sec, PTR fp, bfd_boolean do_set)
{
  asection *sect = p32_start_sec->sec;
  struct pic32_section *crt_sec;
  unsigned char *buf,*p;
  unsigned char **d = (unsigned char **) fp;
  bfd_vma runtime_size = sect->size;
  bfd_vma runtime_addr;
  unsigned char sec_type = 0; /* =1 ->bss, =2 ->data */
  /* keep a local list with the sections that can be merged in dinit */
  struct pic32_section *merged_sec = NULL, *tmp;

  if (sect->output_section == NULL)
    return p32_start_sec->next;

  /* skip persistent or noload data sections - shouldn't happen!!!
   replace with assert */
  if (PIC32_IS_PERSIST_ATTR(sect) || PIC32_IS_NOLOAD_ATTR(sect))
    {
      if (pic32_debug)
        printf("  %s (skipped), size = %x\n",
               sect->name, (unsigned int) runtime_size);
      return p32_start_sec->next;
    }
#ifdef TARGET_IS_PIC32C
  if (!sect->init)
    {
      if (pic32_debug)
      	fprintf (stderr, "data_section %s (skipped, !init)\n", sect->name);
      return p32_start_sec->next;
    }

#endif

  if (sect->size > 0)
    runtime_addr = sect->output_offset + sect->output_section->vma;

  /* check if it is bss or data section*/
  dinit_is_bss_section(sect)? (sec_type = 1) : (sec_type = 2);

  /* check if it is data and initialized with repeated value */
  if (sec_type == 2)
  {
    dinit_type data_type = dinit_get_entry_type(sect, d, do_set);

    if ((data_type == DINIT_COPY_VAL_DATA)
        || (data_type == DINIT_COPY_VAL_EMB))
    {
        /* if the section is initialized with repeated values, dinit header has been generated */
        return p32_start_sec->next;
    }
  }

  /* build the list of consecutive sections that can be merged in dinit */
  /* using a stack address - not a good practice!!!*/
  pic32_init_section_list(&merged_sec);
  pic32_add_section_to_list(merged_sec, sect);

  /* check next section */
  crt_sec = p32_start_sec->next;

  while(crt_sec!= NULL)
  {
    while(crt_sec && !(dinit_is_valid_section (crt_sec->sec)))
    {
      crt_sec = crt_sec->next;
    }

    /* check if the sections can be merged */
    if (crt_sec && are_dinit_compatible(sect, crt_sec->sec))
    {
      /* check if the next one to be added is a good candidate for repeated values */
      if (sec_type == 2)
      {
        dinit_type data_type = dinit_get_entry_type(crt_sec->sec, d, do_set);

        if ((data_type == DINIT_COPY_VAL_DATA) || (data_type == DINIT_COPY_VAL_EMB))
        {
          crt_sec = crt_sec->next;
          break;
        }
      }

      if (pic32_debug)
        printf("  Merging %s , size = %x bytes & %s, size %x\n",
               sect->name, (unsigned int) sect->size,
               crt_sec->sec->name, crt_sec->sec->size) ;

        runtime_size += crt_sec->sec->size;
        /* add the new section in the list */
        pic32_add_section_to_list(merged_sec, crt_sec->sec);
        saved_dinit+= 12; /* number of bytes  - HEADER size*/

        sect = crt_sec->sec;
        crt_sec = crt_sec->next;
    }
    else
        break;
  }

  /*the list is complete - start filling .dinit */
  /* process BSS-type sections */
  if (sec_type == 1)
    {

      /* write header only */
      bfd_pic32_write_data_header(d, runtime_addr, runtime_size, DINIT_CLEAR, do_set);

    }
   /* process DATA-type sections */
  else
    {
      int count = 0;
      /* write header */
      bfd_pic32_write_data_header(d, runtime_addr, runtime_size, DINIT_COPY, do_set);

      for (tmp = merged_sec; tmp != NULL; tmp = tmp->next)
      {
          if (tmp->sec == NULL)
            continue;

        sect = tmp->sec;
        buf = dinit_get_section_contents(sect, do_set);

        if (do_set)
          for (p = buf; p < (buf + sect->size); )
            *(*d)++ = *p++;

      free(buf);
    }
    count = (4 - runtime_size%4)%4;

    /* padding with 0*/
    while (count != 0)
    {
      if (do_set)
        *(*d)++ = 0;

	  count--;
    }

    /* reset sections LOAD flags */
    for (tmp = merged_sec; tmp!= NULL; tmp = tmp->next)
        if (tmp->sec != NULL)
          dinit_reset_section_flags(tmp->sec, do_set);

    } /* process DATA-type sections */

  return crt_sec;
} /* static struct pic32_section* bfd_pic32_process_merged_sections (...)*/


#define     COMPRESSION_GAIN_MIN    50
/*
* find the potential compression gain
* if the gain is over COMPRESSION_GAIN_MIN
* the function returns TRUE, otherwise FALSE 
*/
bfd_boolean is_compressable(PTR fp, unsigned int length)
{
  unsigned char *p, *ahead;
  unsigned char **conv = fp;     
  unsigned char *data = *conv;
  unsigned int gain_bytes = 0;

  if (length <= COMPRESSION_GAIN_MIN)
    return FALSE;

  for (p = data, ahead = p+1; p < (data + length - 1); p++, ahead++)
  {
    if (*p == 0)
      (*ahead == 0)? gain_bytes++:gain_bytes--;
  }

  return (gain_bytes >= COMPRESSION_GAIN_MIN)? TRUE:FALSE;
}

/**
 * Compress the buffer content
 * buf - holds the data to be compressed
 * entry_len - the length of the buffer
 * fp - where the compressed data is stored
 * returns the length of the compressed data
*/
static unsigned int dinit_compress (unsigned char * const buf,
                                    unsigned int entry_len, PTR fp,
                                    bfd_boolean do_set)
{
  int length = 0, nb_zero=0, saved = 0;
  unsigned char **d = fp;
  unsigned char *p;

  for (p = buf; p < (buf + entry_len); p++)
  {
    if(*p == 0)
    {
      if (nb_zero == 0xFF)
      {
        if (do_set)
        {
          *(*d)++ = 0;
          *(*d)++ = 0xFF;
        }

        saved += (nb_zero - 1);
        nb_zero = 1;
        length +=2; /* written bytes: < 0 | 0xFF > */
      }
      else
        nb_zero++;
    }
    else
    {
      if(nb_zero != 0)
      {
        if (do_set)
        {
          *(*d)++ = 0;
          *(*d)++ = nb_zero;
          *(*d)++ = *p;
        }
        nb_zero = 0;
        length += 3; /* written bytes: < 0 | nb_zero | *p > */
      }
      else
      {
        if (do_set)
          *(*d)++ = *p;
        length++;
      }
    }
  }

  if (nb_zero != 0)
  {
    if (do_set)
    {
      *(*d)++ = 0;
      *(*d)++ = nb_zero;
    }
    length += 2; /* written bytes: < 0 | nb_zero > */
  }

  int i = (4 - length%4)%4;

  /* padding with 0*/
  while (i != 0)
  {
    if (do_set)
      *(*d)++ = 0;
    i--;
  }
  return length;
}


static uint32_t
bfd_pic32_compress_data_section(struct pic32_section * const p32_start_sec, PTR fp, bfd_boolean do_set)
{
  asection* sect = p32_start_sec->sec;
  unsigned char *buf,*p;
  unsigned char **d = (unsigned char **) fp;
  bfd_vma runtime_size = sect->size;
  bfd_vma runtime_addr;
  unsigned int count, saved = 0;

  if (sect->output_section == NULL)
    return 0;
  /* skip persistent or noload data sections - shouldn't happen!!!
   replace with assert */
  if (PIC32_IS_PERSIST_ATTR(sect) || PIC32_IS_NOLOAD_ATTR(sect))
    {
      if (pic32_debug)
        printf("  %s (skipped), size = %x\n",
               sect->name, (unsigned int) runtime_size);
      return 0;
    }
#ifdef TARGET_IS_PIC32C
  if (!sect->init)
    {
      if (pic32_debug)
      	fprintf (stderr, "data_section %s (skipped, !init)\n", sect->name);
      return 0;
    }
#endif

  if (sect->size > 0)
    runtime_addr = sect->output_offset + sect->output_section->vma;

  if (dinit_is_bss_section(sect))
  {
    /* write header */
    bfd_pic32_write_data_header(d, runtime_addr, runtime_size, DINIT_CLEAR, do_set);
    return 0;
  }

  buf = dinit_get_section_contents(sect, do_set);
 
  if (is_compressable(&buf, sect->size) /* to be replaced by the compress option */
      && !dinit_is_relocatable(sect))
  {
    bfd_pic32_write_data_header(d, runtime_addr, runtime_size, DINIT_COMPRESSED, do_set);

    /*compress buffer*/
    count = 0; /* padded inside compression */
    saved = (sect->size - dinit_compress (buf,  sect->size, d, do_set)) & 0xFFFFFFFC; /* ensure 4-byte alignment*/
  }
  else
  {
    bfd_pic32_write_data_header(d, runtime_addr, runtime_size, DINIT_COPY, do_set);
    for (p = buf; p < (buf + sect->size); )
    {
      if (do_set)
        *(*d)++ = *p++;
      else
        *p++;
    }
    count = (4 - runtime_size%4)%4;
  }

  /* padding with 0*/
  while (count != 0)
  {
    if (do_set)
      *(*d)++ = 0;

    count--;
  }

  free(buf);

  /* reset flags - make section not LOADable */
  dinit_reset_section_flags(sect, do_set);
  return saved;
}

/* root .dinit function */
void fill_dinit_section(bfd *abfd, struct bfd_link_info *info, bfd_boolean do_set)
{
  asection              *dinit_sec;
  bfd_size_type         dinit_size;
  file_ptr              dinit_offset;
  unsigned char         *dat;
  struct pic32_section  *s_sec;
  uint32_t              i;
  Elf_Internal_Shdr     *hdr;

 #ifdef TARGET_IS_PIC32C
   if (!pic32_data_init)
        return;
 #endif

    BFD_ASSERT (init_template != NULL);

    if (pic32_debug)
      fprintf (stderr, "... filling DINIT pass: %d\n", do_set);

    dinit_sec = init_template->output_section;
    dinit_size = init_template->size;
    dinit_offset = init_template->output_offset;
    saved_dinit =0;

    if (!dinit_sec)
        {
          fprintf( stderr, "Link Error: could not access data template\n");
          abort();
        }

   /* clear SEC_IN_MEMORY flag if inaccurate */
   if ((dinit_sec->contents == 0) && ((dinit_sec->flags & SEC_IN_MEMORY) != 0))
        dinit_sec->flags &= ~ SEC_IN_MEMORY;
    
    if(init_data == NULL)
      init_data = (unsigned char*) malloc(dinit_size * sizeof(unsigned char));
    
    if(init_data == NULL)
    {
      fprintf(stderr, "Link Error: can't allocate dinit buffer\n");
      abort();
    }

   /* update the default fill value */
      dat = init_data;
      for (i=0; i < dinit_size; i++)
        *dat++ *= 2;

  /* scan sections and write data records */
      if (pic32_debug)
          printf("\nDinit: Processing data sections:\n");
      dat = init_data;
      for (s_sec = data_sections; s_sec != NULL; )
      {
        if ((s_sec)
            && (s_sec->sec)
            && ((s_sec->sec->flags & SEC_EXCLUDE) == 0)
            && (s_sec->sec->output_section != bfd_abs_section_ptr)
            && (s_sec->sec->size > 0)) {
          /*
           * Set was_reloc before anything else, this attribute persists
           * across both passes ensuring we do not optimize relocatable
           * sections.
           */
          if (dinit_is_relocatable(s_sec->sec))
          {
            if (pic32_debug)
              printf("Relocatable .dinit section: %s\n", s_sec->sec->name);
            s_sec->sec->was_reloc = TRUE;
            s_sec->sec->output_section->was_reloc = TRUE;
          }
          else
          {
            if (pic32_debug)
              printf("Normal .dinit section: %s\n", s_sec->sec->name);
          }

          if (dinit_compress_level == 0)
           {
            bfd_pic32_process_data_section(s_sec->sec, &dat, do_set);
            s_sec = s_sec->next;
           }
          else if (dinit_compress_level == 1)
          {
            if (pic32_debug)
              printf("Saved .dinit bytes pre repeat  : %d\n", saved_dinit);
            s_sec =  bfd_pic32_process_merged_sections(s_sec, &dat, do_set);
            if (pic32_debug)
              printf("Saved .dinit bytes post repeat : %d\n", saved_dinit);
            if (s_sec == NULL)
              break;
          }
          else if (dinit_compress_level == 2)
          {
            if (pic32_debug)
              printf("Saved .dinit bytes pre compress  : %d\n", saved_dinit);
            saved_dinit += bfd_pic32_compress_data_section(s_sec, &dat, do_set);
            if (pic32_debug)
              printf("Saved .dinit bytes post compress : %d\n", saved_dinit);
            s_sec = s_sec->next;
          }
        }
        else
          s_sec = s_sec->next;
      }

#ifdef TARGET_IS_PIC32MX
     if (pic32_code_in_dinit) {
#endif
       if (pic32_debug)
         printf("\nDinit: Processing code sections:\n");

       for (s_sec = code_sections; s_sec != NULL; s_sec = s_sec->next)
         if ((s_sec->sec) && (((s_sec->sec->flags & SEC_EXCLUDE) == 0) &&
             (s_sec->sec->output_section != bfd_abs_section_ptr)))
#if TARGET_IS_PIC32C
	    if (s_sec->sec->itcm)
#endif
		    bfd_pic32_process_code_section(s_sec, &dat, TRUE, do_set);
#ifdef TARGET_IS_PIC32C
	    else if (pic32c_relocate_vectors
		       && (strstr (s_sec->sec->output_section->name, ".vectors")
			   != NULL))
		    bfd_pic32_process_code_section (s_sec, &dat, FALSE, do_set);

#endif
#ifdef TARGET_IS_PIC32MX
     }
#endif
     if (do_set)
     {
     /* write zero terminator */
       *dat++ = 0; *dat++ = 0;
       *dat++ = 0; *dat++ = 0;
/* as 0x0 (32b) is a valid address on PIC32C devices, the end of .dinit section is marked with 64b */
#ifdef TARGET_IS_PIC32C
       *dat++ = 0; *dat++ = 0;
       *dat++ = 0; *dat++ = 0;
#endif
     }


#ifdef TARGET_IS_PIC32MX
     if (do_set)
     {
       if (pic32_code_in_dinit)
       {

         struct bfd_link_hash_entry *h;
         h = bfd_link_hash_lookup (info->hash, entry_symbol_copy.name,
                                   FALSE, FALSE, TRUE);
         if (h != NULL
             && (h->type == bfd_link_hash_defined
                 || h->type == bfd_link_hash_defweak)
             && h->u.def.section->output_section != NULL)
         {
          bfd_vma entry_address;

          entry_address = h->u.def.value
                          + bfd_get_section_vma (info->output_bfd,
                                     h->u.def.section->output_section)
                          + h->u.def.section->output_offset;


          *dat++ = entry_address & 0xFF;
          *dat++ = (entry_address >> 8) & 0xFF;
          *dat++ = (entry_address >> 16) & 0xFF;
          *dat++ = (entry_address >> 24) & 0xFF;
         }
       }
     }
#endif

    if (pic32_debug)
        printf("  last template addr written = %lx\n",
                 (long unsigned int)(dat-1));

    /* set the new dinit size */
    if (!do_set)
    {
      dinit_sec->size -= saved_dinit;
      dinit_sec->rawsize -= saved_dinit;
    }

    hdr = &elf_section_data (dinit_sec)->this_hdr;
    hdr->sh_size = dinit_sec->size;
    dinit_sec->flags |= SEC_LINKER_CREATED;
    init_template->flags |= SEC_LINKER_CREATED;
}

void set_dinit_content(bfd *abfd, struct bfd_link_info *info)
{
  asection              *dinit_sec;
  bfd_size_type         dinit_size;
  file_ptr              dinit_offset;
  fill_dinit_section(abfd, info, TRUE);

  #ifdef TARGET_IS_PIC32C
    if (!pic32_data_init)
         return;
  #endif

  dinit_sec = init_template->output_section;
  dinit_size = init_template->size - saved_dinit;
  dinit_offset = init_template->output_offset;

  bfd_seek(dinit_sec->owner, 0, SEEK_SET);

  if (!bfd_set_section_contents (abfd, dinit_sec,
                                 init_data, dinit_offset, dinit_size))
  {
      fprintf( stderr, "Link Error: can't write section %s contents offset -> %d \n",
                  dinit_sec->output_section->name);
      abort();
  }
}

#endif /* __PIC32_DINIT__ */
