# P02 firmware update: possible additions

Appendix to [P02 firmware update flow](flow_en.md). Status **Option**: possible addition, if the need arises. Not planned. Taken on if stakeholders see a need.

Svenska: [possible_additions_sv.md](possible_additions_sv.md)

## The RAM-load variant: firmware that does not fit in the internal flash (Option)

Extend the module with RAM load, for products whose firmware does not fit in the chip's internal flash. The
slots are then in external flash (QSPI), and MCUboot copies the image into RAM and runs it there. That is the
case on the Daisy (STM32H750: 128 KB internal flash, P02 ~133 KB).

* Requires that we first decide what test, confirm and revert mean in RAM load. Today the Daisy's RAM load has
  no rollback (open item in AGENTS.md).
* Requires that the USB fault after a software reset on the Daisy is solved, since every update ends with a
  software reset.
* May become relevant for the second product, if its firmware is large.

## Forbidden versions (Option)

A list of old versions that have known faults and that an update to is not allowed. P02 compares the version
in the header with the list already in the first chunk (the upload check hook) and rejects the upload with an
error code to the client.

* The list is built into the firmware. It therefore only applies from the version that has it, and an older
  version does not know about later additions.
* Differs from the check "higher version than the running one" (`upgrade`), which the client chooses. The
  list always applies.
* Only protects uploads through P02. Flashing and a revert by MCUboot bypass it.

## Checking the signature on reception (Option)

P02 checks the signature when the whole image has arrived (the status hook, `DFU_PENDING`), with the same
public key as MCUboot. An image with a wrong signature is then caught before the restart, not only when
MCUboot refuses to start it. The reset hook can decline the restart, and the result can be shown in
`image state-read`.

* Requires the iteration *Signed images* (today `BOOT_SIGNATURE_TYPE_NONE`).
* The same check is done twice. MCUboot's check at startup is still the one that counts.
* Requires crypto code in P02 (ECDSA P-256), which costs flash. Especially important on the Nucleo with its
  200 KB slot.

## The right product, not only the right board (Option)

The board check only compares the board target. Two products on identical boards, for example a windscreen
wiper and a cabin heater, have the same board target, so an image for the windscreen wiper is accepted and
started on the cabin heater. Do we also want to check that it is the right product?

* One way: a product ID in a TLV of its own, like the board target, compared with the product ID in the
  running firmware. Each product sets the ID (for example a Kconfig option in the module,
  `CONFIG_FW_UPDATE_PRODUCT_ID`), and the check is done in the same hook as the board check, with the same
  erase and line on the serial port.
* Another way: one signing key per product, once *Signed images* exists. MCUboot then refuses to start an
  image for another product, but only at the restart, not when it is received.
* To decide: whether product and board are one check (one TLV with both) or two, and how a board
  intentionally changes product (flashing, which bypasses the check).

## Monitoring the transfer (Option)

The module measures the response time per chunk: the time from `smp_rx_req()` (main) until mcumgr calls
`output()` with the response (mcumgr's thread). Since only one chunk is in flight at a time, one timestamp is
enough.

* The module stores the relevant information so that it can be read with the command `status`.
* No logging during the upload: `smpmgr` has the port then, so a line would end up at the client and not with
  a person.
* An addition: rename `restartRequested()` to an explicit `poll()`, which the module can use for periodic
  work. The module could then also detect a request that never gets a response, and for example clean up a
  half-finished upload. The client gets a timeout in that case anyway.
* Also provides the measurement for the Nano verification in phase 2 of [the flow](flow_en.md): how much the flash writes stop the
  threads.

## C or C++ interface to the module (Option)

The module has a C API today (see Background and the decision *Zephyr module* in [the flow](flow_en.md)). A C++ interface is a possible
alternative, if the second product and P02 are both written in C++.

* For C: usable from both C and C++, which suits a product whose language is not known yet. mcumgr's
  callbacks and transport API are C function pointers, so no translation is needed. The module cannot
  accidentally pull in P02's classes. In P02 the C functions are only called inside `boot::Zephyr` and
  `serial::Zephyr`, so `Logic` still only sees `boot::Interface`.
* For C++: the same style as P02 (classes, `noexcept`, no function pointers), and the callback `tx` can be an
  interface instead of a function pointer with `void* user`.
* Cost with C++: every product using the module needs C++ (`CONFIG_CPP`), which costs flash.
* A middle ground: the C API in the module, and a thin C++ header on top for products in C++.
