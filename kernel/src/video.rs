use spin::Mutex;

derive_kernel_enum! {
    pub enum LineType {
        HorizontalLine,
        VerticalLine,
    }
}

// From UEFI Specification
derive_kernel_enum! {
    pub enum EfiGraphicsPixelFormat {
        PixelRedGreenBlueReserved8BitPerColor,
        PixelBlueGreenRedReserved8BitPerColor,
        PixelBitMask,
        PixelBltOnly,
        PixelFormatMax,
    }
}

#[derive(Debug, Clone, Copy,)]
#[repr(C)]
pub struct BootVideoInfo {
  pub base_address: *mut u32,
  pub buffer_size: usize,
  pub width: u32,
  pub height: u32,
  pub pixels_per_scanline: u32,
  pub pixel_format: EfiGraphicsPixelFormat,
}

unsafe impl Send for BootVideoInfo {}

impl BootVideoInfo {
  pub unsafe fn draw_pixel(&self, x: usize, y: usize, color: u32,) {
    if x < self.width as usize && y < self.height as usize
    {
      let offset = y * (self.pixels_per_scanline as usize) + x;

      unsafe {
        *self.base_address.add(offset,) = color;
      }
    }
  }

  pub unsafe fn fill_screen(&self, color: u32,) {
    let total_pixels = (self.pixels_per_scanline as usize) * (self.height as usize);

    for i in 0..total_pixels
    {
      unsafe {
        *self.base_address.add(i,) = color;
      }
    }
  }

  pub unsafe fn draw_line(
    &self,
    x: usize,
    y: usize,
    length: usize,
    color: u32,
    type_line: LineType,
  ) {
    match type_line
    {
      LineType::HorizontalLine =>
      {
        let max_x = (x + length).min(self.width as usize,);
        for current_x in x..max_x
        {
          unsafe {
            self.draw_pixel(current_x, y, color,);
          }
        }
      }
      LineType::VerticalLine =>
      {
        let max_y = (y + length).min(self.height as usize,);
        for current_y in y..max_y
        {
          unsafe {
            self.draw_pixel(x, current_y, color,);
          }
        }
      }
    }
  }

  pub unsafe fn draw_rectangle(
    &self, x: usize, y: usize, width: usize, height: usize, color: u32,
  ) {
    let max_x = (x + width).min(self.width as usize,);
    let max_y = (y + height).min(self.height as usize,);

    for current_y in y..max_y
    {
      for current_x in x..max_x
      {
        unsafe {
          self.draw_pixel(current_x, current_y, color,);
        }
      }
    }
  }
}

pub static FRAMEBUFFER: Mutex<Option<BootVideoInfo,>,> = Mutex::new(None,);

pub fn draw_pixel_global(x: usize, y: usize, color: u32,) {
  if let Some(fb,) = FRAMEBUFFER.lock().as_ref()
  {
    unsafe {
      fb.draw_pixel(x, y, color,);
    }
  }
}

pub fn fill_screen_global(color: u32,) {
  if let Some(fb,) = FRAMEBUFFER.lock().as_ref()
  {
    unsafe {
      fb.fill_screen(color,);
    }
  }
}

pub fn draw_line_global(x: usize, y: usize, length: usize, color: u32, type_line: LineType,) {
  if let Some(fb,) = FRAMEBUFFER.lock().as_ref()
  {
    unsafe {
      fb.draw_line(x, y, length, color, type_line,);
    }
  }
}

pub fn draw_rectangle_global(x: usize, y: usize, width: usize, height: usize, color: u32,) {
  if let Some(fb,) = FRAMEBUFFER.lock().as_ref()
  {
    unsafe {
      fb.draw_rectangle(x, y, width, height, color,);
    }
  }
}
