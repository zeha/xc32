# name: attributes for -march=tag13
# source: blank.s
# as: -march=tag13
# readelf: -A
# This test is only valid on EABI based ports.
# target: *-*-*eabi

Attribute Section: aeabi
File Attributes
  Tag_CPU_name: "tag13"
  Tag_CPU_arch: \?\?\? \(13\)
  Tag_CPU_arch_profile: Microcontroller
  Tag_THUMB_ISA_use: Thumb-2
