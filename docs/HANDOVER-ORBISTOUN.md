# Handover: what obSCEne measured, and a process it suggests

Written from obSCEne on 2026-08-25, for the orbistoun side to act on or refuse. It assumes no
knowledge of the conversation that produced it.

Everything in the first half is measured. The second half is a proposal, and the parts of it
that would need a person are marked.

---

## 1. What obSCEne measured on orbistoun

**orbistoun runs the whole suite.** 36,549 records, 27 of 27 sections, 515 of 515 checks, in
about two seconds - 416 pass, 16 partial, 56 fail, 27 skip. It loads a bare current-generation
ELF, resolves imports, executes, and writes a complete report. Of the five loaders obSCEne is
run against, only two others complete the suite that cleanly.

**Two functions block everything visual, and only those two:**

```text
070-user/initialise    fail  0x7fff0001  the user service refused to initialise
070-user/initial-user  fail  0x7fff0001  no initial user could be determined
OBS|display|absent|no initial user, even after initialising the service
```

`0x7fff0001` is orbistoun's own Unimplemented placeholder, correctly carrying no high bit. So
this is the loader saying *not implemented*, not *refused*, and obSCEne is behaving correctly
in response: it opens its display against a user, and with no user it declines to open one at
all rather than guessing an id.

The consequence is narrow and total. obSCEne draws its report to the screen - that drawing is
the only report there is on a loader with no working text channel - and on orbistoun there is
nothing to photograph. Every other loader in the toolkit renders it. Nothing else is blocked:
the run completes and the text report is full.

**So `sceUserServiceInitialize` and `sceUserServiceGetInitialUser` are the entire distance
between orbistoun and a rendered frame.**

---

## 2. The value is genuinely unknown, and that is the useful part

Three independent implementations of `sceUserServiceGetInitialUser` were compared. **All three
return a different constant, and no two agree.** The values are deliberately not reproduced
here: an implementation copying any one of them is copying a guess, and knowing *that they
disagree* is worth more than knowing what they chose.

One thing is corroborated across two of them: a distinct **system-user sentinel** exists,
separate from any real user id, and it is the value their display code expects. That is a real
constraint on the model rather than a value to copy.

**A worked example of what happens without a model.** In one of those emulators
`GetInitialUser` returns one constant and its own `sceVideoOutOpen` rejects that exact value -
two functions in the same build, mutually exclusive, because each picked a number
independently on a different day. It is not a careless project. It is what picking scalars
instead of deriving them costs, and it is the argument for bones over a stub.

---

## 3. The reframe this suggests

The loop's finding today is **function-shaped**:

> `sceUserServiceGetInitialUser` is unimplemented

which invites a function-shaped fix - return a number - and that is the failure above.

The same run already holds enough to say something **library-shaped**:

> `libSceUserService`: N names recovered, M carrying plurality or lifecycle markers.
> No backing model exists. All entry points Unimplemented.

That tells a person what to build without telling them how, and it is derivable from data
orbistoun already banks.

---

## 4. Where the shape comes from, without reading anyone's implementation

### 4a. The recovered names are the specification

Names are confirmed by hashing candidates against the NID. Nothing is read from another
project, and the names are ABI identifiers the guest itself imports. The vendor's own
vocabulary encodes the design:

| name element | what it commits to |
|---|---|
| `…IdList` | a list of ids - plural, enumerable |
| `Initial…` | "initial" is meaningless unless others exist |
| `…Event` | membership changes over time |
| `…Name` | entities carry identity beyond an id |

**Suggested change:** a pass over each library's *confirmed* name set that classifies it -
scalar, collection, or collection-with-lifecycle - from marker words (`List`, `Count`, `Num`,
`Enum`, `Next`, `Index`, `Id`; `Event`, `Add`, `Remove`, `Open`, `Close`). This is a new
reading of data already held, not new collection, and it emits a **finding**, not an
implementation.

### 4b. Watchpoints already answer the arity questions

The existing pipeline is *fault → snapshot the structure → diff → arm up to four words →
named instruction offset*. Pointed at an **out-buffer** rather than a fault address, the same
machinery measures:

- how many slots the guest reads before stopping → **capacity**
- the value it stopped on → **the invalid sentinel**
- the stride between reads → **element size**

Nothing is proposed here, so by the rule in `orbistoun-propose` no oracle is required - it is
measurement, and it lands as `known_by: guest-observed`.

### 4c. obSCEne is a known-input generator, and neither project uses it that way

A title may only ever call `GetInitialUser`. obSCEne calls entry points **deliberately, one at
a time, announcing each by name before the call** - that is its whole announce-before-attempting
design. Running orbistoun's observer against obSCEne therefore yields a labelled, isolated call
with known arguments, for the *whole* library, including entry points no title touches.

If obSCEne should probe this library harder to make that useful - sweeping the user-service
surface the way it sweeps mutex types - say so and it will be added. That is obSCEne's side of
the work and it is cheap.

---

## 5. Where a model fits, and where it must not

The measured position in `THE_LOOP.md` is not being argued with: selection lost to exhaustion,
and `vocabulary` works because the NID hash is a free oracle that cannot be talked into
agreeing.

There is exactly one task above with that same shape: **proposing the marker words** in 4a. Is
`Slot` a collection marker? `Handle`? `Entry`? The oracle is a grep over confirmed names, a
wrong suggestion costs nothing, and it is a grammar problem - the one thing a model has been
measurably useful for in this project. That is the same trust already granted, not a new one.

**A model should not choose what to implement, and should not write the implementation.**

---

## 6. What stays a person, and why the decision gets smaller

`THE_LOOP.md` is right that step 18 is a real decision and should not be generated. The point
of everything above is not to remove the decision but to shrink it:

| | the decision presented |
|---|---|
| today | "`GetInitialUser` is unimplemented" → **pick a number** |
| with the above | "collection, identity, lifecycle, observed capacity N, sentinel observed" → **build the bones** |

In the second, the id is *derived from the model* rather than invented, so it is automatically
consistent everywhere it appears - which is exactly the invariant the emulator in §2 violated.

Minimal bones would be a user table with one occupant, `GetLoginUserIdList` returning that one,
`GetUserName` returning something stable, and `GetEvent` reporting no transitions. Not because
the hardware has one user, but because the *shape* is then right and a second user is data
rather than surgery.

**The values stay `assumed`.** They are exactly what `orbistoun-cli questions --json` ranks and
what a hardware probe settles, and both ends of that are already built. obSCEne can carry those
questions to hardware when it is available.

---

## 7. If only one thing is taken from this

The finding that matters is not "implement two functions". It is that **a library's recovered
names already state whether it models one thing or many**, and orbistoun holds those names
before it executes a single guest instruction. Reading them that way turns a scalar stub into
a shaped one, and it needs no hardware, no other project's source, and no model.
