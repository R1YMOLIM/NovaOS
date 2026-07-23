// src/macros.rs

macro_rules! derive_kernel_enum {
  ($item:item) => {
    #[derive(Debug, Clone, Copy, PartialEq, Eq,)]
    #[repr(C)]
    $item
  };
}
