# D231 - `EBOOT_KIND`, so the same eboot can be put through an emulator first


The eboot carries `e_type 0xFE00` because a console's `rtld` refuses `0xFE10` by name. Every
emulator refuses `0xFE00` in return - PS5PCEM's `module-info` says `UnsupportedObjectType`.

Four bytes, and they meant the shape being changed was the one shape that could not be tested
anywhere except on the console. `EBOOT_KIND=executable` stamps the other value and changes
nothing else: same sources, same twelve libraries, same run-time census.

That is not a convenience. D232 below is a regression that would have reached hardware.

