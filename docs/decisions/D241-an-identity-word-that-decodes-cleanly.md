# D241 - An identity word that decodes cleanly is not a well-formed identity word


*status: decided*

`import_libs` printed our library table and it was perfect: ids dense from zero, names right,
versions consistent, nothing out of range. Every field a human would check.

It prints the **raw word** beside the decode now, and the attribute word beside the identity
word, because neither of those was shown and one of them was wrong. Every import library in a
real launching title carries attribute `0x9`; this crate wrote `0x1`, reusing the *export*
attribute constant for the import side because the two words are the same shape and one
constant covered both.

Fixing it changed nothing on hardware - measured, not assumed - so it is not the binding
defect. It is still wrong, it is now right, and it has a test pinned to the literal rather than
to the constant, because asserting a constant equals itself passes whatever it becomes.

The general form: a probe that shows only the fields it knows how to name will show a malformed
structure as an ordinary one. Print the raw word.

