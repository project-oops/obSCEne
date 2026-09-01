# D254 - Double-buffering prevents screen tearing during report rendering


*status: decided*

With a single framebuffer, drawing directly into the memory being scanned out causes visible tearing and flickers as lines and boxes render. Two buffers eliminate tearing by allowing a completed frame to be submitted to the display before drawing the next frame.

