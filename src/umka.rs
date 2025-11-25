#![allow(warnings)]

use std::ffi::{CStr, CString, c_char, c_double, c_float, c_int, c_longlong, c_ulonglong, c_void};
use std::ptr;

use crate::to_cstr;
use crate::rl::{Color, Vector2};

type UmkaWarningCallback = unsafe extern "C" fn(*mut UmkaError);

#[link(name = "umka")]
unsafe extern "C" {
    fn umkaAlloc() -> *mut c_void;
    fn umkaInit(
        umka: *mut c_void,
        fileName: *const c_char,
        sourceString: *const c_char,
        stackSize: c_int,
        reserved: *mut c_void,
        argc: c_int,
        argv: *mut *mut c_char,
        fileSystemEnabled: bool,
        implLibsEnabled: bool,
        warningCallback: *mut UmkaWarningCallback,
    ) -> bool;
    fn umkaCompile(umka: *mut c_void) -> bool;
    fn umkaCall(umka: *mut c_void, func: *mut UmkaFuncContext) -> c_int;
    fn umkaGetError(umka: *mut c_void) -> *mut UmkaError;

    fn umkaAddModule(
        umka: *mut c_void,
        fileName: *const c_char,
        sourceString: *const c_char,
    ) -> bool;
    fn umkaGetFunc(
        umka: *mut c_void,
        moduleName: *const c_char,
        fnName: *const c_char,
        func: *mut UmkaFuncContext,
    ) -> bool;

    fn umkaGetParam(params: *mut UmkaStackSlot, index: c_int) -> *mut UmkaStackSlot;
    fn umkaGetResult(params: *mut UmkaStackSlot, result: *mut UmkaStackSlot) -> *mut UmkaStackSlot;
}

#[repr(C)]
pub struct UmkaError {
    file_name: *const c_char,
    fn_name: *const c_char,
    line: c_int,
    pos: c_int,
    code: c_int,
    msg: *const c_char,
}

#[derive(Default)]
#[repr(C)]
struct UmkaFuncContext {
    entry_offset: c_longlong,
    params: *mut UmkaStackSlot,
    result: *mut UmkaStackSlot,
}

#[derive(Copy, Clone)]
#[repr(C)]
union UmkaStackSlot {
    int_val: c_longlong,
    uint_val: c_ulonglong,
    ptr_val: *mut c_void,
    real_val: c_double,
    real32_val: c_float,
}

enum UmkaSlotKind {
    Real32,
    Real,
    Vec2,
    Color
}

#[derive(Clone, Debug)]
enum UmkaValue {
    Real32(f32),
    Real(f64),
    Vec2(Vector2),
    Color(Color),
}

impl UmkaValue {
    fn from_slot(slot: UmkaStackSlot, kind: UmkaSlotKind) -> Self {
        // NOTE: when a function returns a real32 in umka, it shows up as
        // real on the C or rust side; hence, the code below...
        unsafe {
            match kind {
                UmkaSlotKind::Real32 => {
                    let UmkaStackSlot{real_val: r} = slot;
                    Self::Real32(r as f32)
                },
                UmkaSlotKind::Real => {
                    let UmkaStackSlot{real_val: r} = slot;
                    Self::Real(r)
                },
                UmkaSlotKind::Vec2 => {
                    let UmkaStackSlot{ptr_val: ptr} = slot;
                    Self::Vec2(*(ptr as *mut Vector2))
                },
                UmkaSlotKind::Color => {
                    let UmkaStackSlot{ptr_val: ptr} = slot;
                    Self::Color(*(ptr as *mut Color))
                },
                _ => unimplemented!()
            }
        }
    }

    fn to_slot(mut self) -> UmkaStackSlot {
        unsafe {
            match self {
                UmkaValue::Real32(r) => UmkaStackSlot { real32_val: r },
                // UmkaValue::Vec2(ref mut v) => UmkaStackSlot { ptr_val: v as *mut _ as *mut c_void },
                UmkaValue::Vec2(mut v) => {
                    // NOTE: Pass by value
                    // NOTE: This feels like some rust dark arts...
                    let a = &mut self as *mut _ as *mut Vector2;
                    *a = v;
                    *(a as *mut _ as *mut UmkaStackSlot)
                }
                UmkaValue::Color(mut c) => {
                    let a = &mut self as *mut _ as *mut Color;
                    *a = c;
                    *(a as *mut _ as *mut UmkaStackSlot)
                }
                _ => unimplemented!()
            }
        }
    }
}

struct Umka {
    ptr: *mut c_void,
    file_name: String,
}

impl Umka {
    fn init(file_name: &str) -> Self {
        let ptr = unsafe {
            let umka = umkaAlloc();
            let umka_ok = umkaInit(
                umka,
                to_cstr!(file_name),
                ptr::null_mut(),
                1024 * 1024,
                ptr::null_mut(),
                0,
                ptr::null_mut(),
                false,
                false,
                ptr::null_mut::<UmkaWarningCallback>(),
            );
            assert!(umka_ok);
            umka
        };
        Self {
            ptr,
            file_name: file_name.to_string(),
        }
    }

    fn handle_error(&self) {
        unsafe {
            let err = umkaGetError(self.ptr);
            let file_name = str::from_utf8(CStr::from_ptr((*err).file_name).to_bytes()).unwrap();
            let msg = str::from_utf8(CStr::from_ptr((*err).msg).to_bytes()).unwrap();
            eprintln!("{}:{}:{:?}: {}", file_name, (*err).line, (*err).pos, msg);
        }
        panic!("Look at error above!");
    }

    fn compile(&self) -> bool {
        let success = unsafe { umkaCompile(self.ptr) };
        if !success {
            self.handle_error();
        }
        success
    }

    fn add_module(&self, mod_file_name: &str, source_str: &str) -> bool {
        let success =
            unsafe { umkaAddModule(self.ptr, to_cstr!(mod_file_name), to_cstr!(source_str)) };
        if !success {
            self.handle_error();
        }

        success
    }

    fn get_func(&self, fn_name: &str) -> Option<UmkaFuncContext> {
        let mut func: UmkaFuncContext = Default::default();
        let success =
            unsafe { umkaGetFunc(self.ptr, ptr::null_mut(), to_cstr!(fn_name), &mut func) };
        if !success {
            self.handle_error();
            return None;
        }
        Some(func)
    }

    fn call(&self, func: &mut UmkaFuncContext) -> i32 {
        let result = unsafe { umkaCall(self.ptr, func) };
        if result != 0 {
            self.handle_error();
        }
        result
    }

    fn set_param(&self, fcx: &mut UmkaFuncContext, index: i32, value: UmkaValue) {
        unsafe {
            let mut param = umkaGetParam(fcx.params, index);
            (*param) = value.to_slot();
        }
    }

    fn get_result(&self, fcx: &mut UmkaFuncContext, kind: UmkaSlotKind) -> UmkaValue {
        let slot = unsafe { *umkaGetResult(fcx.params, fcx.result) };
        UmkaValue::from_slot(slot, kind)
    }

    fn set_result(&self, fcx: &mut UmkaFuncContext, value: &mut UmkaValue) {
        unsafe {
            let result = umkaGetResult(fcx.params, fcx.result);
            (*result).ptr_val = value as *mut _ as *mut c_void;
        }
    }
}

pub fn umka_test() {
    let path = "./test.um";
    let umka = Umka::init(path);
    umka.add_module("t.um", "type vec2* = struct { x, y: real32 }\ntype color* = struct { r, g, b, a: uint8 }");
    // umka.add_module("t.um", "type vec2* = struct { x, y: real32 }");
    // umka.add_module("t.um", "type color* = struct { r, g, b, a: uint8 }");
    umka.compile();

    if let Some(mut fcx) = umka.get_func("color_disappear") {
        umka.set_param(&mut fcx, 0, UmkaValue::Color(Color::BLUE));
        let mut out = UmkaValue::Color(Color::BLACK);
        umka.set_result(&mut fcx, &mut out);
        umka.call(&mut fcx);
        let res = umka.get_result(&mut fcx, UmkaSlotKind::Color);
        println!("Result: {:?}", res);
    } else {
        println!("Not your day!");
    }
}
