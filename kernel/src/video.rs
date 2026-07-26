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
    }
}

#[derive(Debug, Clone, Copy,)]
#[repr(C)]
pub struct BootVideoInfo {
  base_address: *mut u32,
  buffer_size: usize,
  width: u32,
  height: u32,
  pixels_per_scanline: u32,
  pixel_format: EfiGraphicsPixelFormat,
}

pub struct Color {
  pub r: u8,
  pub g: u8,
  pub b: u8,
}

unsafe impl Send for BootVideoInfo {}

impl BootVideoInfo {
  pub fn convert_pixel_format(&self, color: Color,) -> u32 {
    let format = self.pixel_format;
    match format
    {
      EfiGraphicsPixelFormat::PixelBlueGreenRedReserved8BitPerColor =>
      {
        ((color.b as u32) << 16) | ((color.g as u32) << 8) | (color.r as u32)
      }
      EfiGraphicsPixelFormat::PixelRedGreenBlueReserved8BitPerColor =>
      {
        ((color.r as u32) << 16) | ((color.g as u32) << 8) | (color.b as u32)
      }
      EfiGraphicsPixelFormat::PixelBitMask => 0,
    }
  }

  pub unsafe fn draw_pixel(&self, x: usize, y: usize, color: Color,) {
    let pixel = self.convert_pixel_format(color,);
    if x < self.width as usize && y < self.height as usize
    {
      let offset = y * (self.pixels_per_scanline as usize) + x;

      unsafe {
        self.base_address.add(offset,).write(pixel,);
      }
    }
  }

  pub unsafe fn fill_screen(&self, color: Color,) {
    let total_pixels = (self.pixels_per_scanline as usize) * (self.height as usize);
    let pixel = self.convert_pixel_format(color,);

    for i in 0..total_pixels
    {
      unsafe {
        self.base_address.add(i,).write(pixel,);
      }
    }
  }

  pub unsafe fn draw_line(
    &self,
    x: usize,
    y: usize,
    length: usize,
    color: Color,
    type_line: LineType,
  ) {
    let pixel = self.convert_pixel_format(color,);
    match type_line
    {
      LineType::HorizontalLine =>
      {
        let max_x = (x + length).min(self.width as usize,);
        for current_x in x..max_x
        {
          let offset = y * self.pixels_per_scanline as usize + current_x;
          unsafe {
            self.base_address.add(offset,).write(pixel,);
          }
        }
      }
      LineType::VerticalLine =>
      {
        let max_y = (y + length).min(self.height as usize,);
        for current_y in y..max_y
        {
          let offset = current_y * self.pixels_per_scanline as usize + x;
          unsafe {
            self.base_address.add(offset,).write(pixel,);
          }
        }
      }
    }
  }

  pub unsafe fn draw_rectangle(
    &self,
    x: usize,
    y: usize,
    width: usize,
    height: usize,
    color: Color,
  ) {
    let max_x = (x + width).min(self.width as usize,);
    let max_y = (y + height).min(self.height as usize,);
    let pixel = self.convert_pixel_format(color,);

    for current_y in y..max_y
    {
      for current_x in x..max_x
      {
        unsafe {
          let offset = current_y * (self.pixels_per_scanline as usize) + current_x;
          self.base_address.add(offset,).write(pixel,);
        }
      }
    }
  }
}

pub static FRAMEBUFFER: Mutex<Option<BootVideoInfo,>,> = Mutex::new(None,);

pub fn draw_pixel_global(x: usize, y: usize, color: Color,) {
  if let Some(fb,) = FRAMEBUFFER.lock().as_ref()
  {
    unsafe {
      fb.draw_pixel(x, y, color,);
    }
  }
}

pub fn fill_screen_global(color: Color,) {
  if let Some(fb,) = FRAMEBUFFER.lock().as_ref()
  {
    unsafe {
      fb.fill_screen(color,);
    }
  }
}

pub fn draw_line_global(x: usize, y: usize, length: usize, color: Color, type_line: LineType,) {
  if let Some(fb,) = FRAMEBUFFER.lock().as_ref()
  {
    unsafe {
      fb.draw_line(x, y, length, color, type_line,);
    }
  }
}

pub fn draw_rectangle_global(x: usize, y: usize, width: usize, height: usize, color: Color,) {
  if let Some(fb,) = FRAMEBUFFER.lock().as_ref()
  {
    unsafe {
      fb.draw_rectangle(x, y, width, height, color,);
    }
  }
}
