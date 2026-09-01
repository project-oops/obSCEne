# D030 - The string table is the first entry in the dynamic table, and everything that names something comes last


Status: decided, on evidence.

A loader walks the dynamic table once, in order, and resolves a name offset the moment
it meets one. Four tags carry a name offset rather than a pointer - the module's own
name, its exported library, and the name of every module and library it imports from.
Emitted before `DT_SCE_STRTAB`, they are offsets into a string table the loader has not
been given, and it dereferences a base it does not have.

The symptom is a fault inside the loader, with nothing in its log, before a single
guest instruction runs. It is indistinguishable from every other way a module can be
malformed, which is what made it expensive.

**How it was found.** By cutting the dynamic table down one entry at a time and running
each truncation. A table terminated immediately ran - the guest reached its spin loop.
A table with one entry did not. That entry was `DT_SCE_MODULE_INFO`, carrying the
module's own name at string-table offset 1.

Pinned by a test rather than a comment: this is exactly the ordering a tidy-up undoes.

