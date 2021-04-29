#include "h.h"

typedef struct BFn {
  struct Fun;
  B ident;
} BFn;
static inline B  mv(B*     p, usz n) { B r = p  [n]; p  [n] = m_f64(0); return r; }
static inline B hmv(HArr_p p, usz n) { B r = p.a[n]; p.a[n] = m_f64(0); return r; }
B eachd_fn(BBB2B f, B fo, B w, B x) { // consumes w,x; assumes at least one is array
  if (!isArr(w)) w = m_hunit(w);
  if (!isArr(x)) x = m_hunit(x);
  ur wr = rnk(w); BS2B wget = TI(w).get;
  ur xr = rnk(x); BS2B xget = TI(x).get;
  bool wg = wr>xr;
  ur rM = wg? wr : xr;
  ur rm = wg? xr : wr;
  if (rM==0) {
    B r = f(fo, wget(w,0), xget(x,0));
    dec(w); dec(x);
    return m_hunit(r);
  }
  if (rm && !eqShPrefix(a(w)->sh, a(x)->sh, rm)) thrM("Mapping: Expected equal shape prefix");
  bool rw = rM==wr && ((v(w)->type==t_harr) & reusable(w)); // v(…) is safe as rank>0
  bool rx = rM==xr && ((v(x)->type==t_harr) & reusable(x));
  if (rw|rx && (wr==xr | rm==0)) {
    HArr_p r = harr_parts(rw? w : x);
    usz ria = r.c->ia;
    if      (wr==0) { B c=wget(w, 0); for(usz i = 0; i < ria; i++) r.a[i] = f(fo, inc(c),   hmv(r,i)); dec(c); }
    else if (xr==0) { B c=xget(x, 0); for(usz i = 0; i < ria; i++) r.a[i] = f(fo, hmv(r,i), inc(c)  ); dec(c); }
    else {
      assert(wr==xr);
      if (rw) for (usz i = 0; i < ria; i++) r.a[i] = f(fo, hmv(r,i),  xget(x,i));
      else    for (usz i = 0; i < ria; i++) r.a[i] = f(fo, wget(w,i), hmv(r,i));
    }
    dec(rw? x : w);
    return r.b;
  }
  
  B bo = wg? w : x;
  usz ria = a(bo)->ia;
  usz ri = 0;
  HArr_p r = m_harrs(ria, &ri);
  if (wr==xr)                       for(; ri < ria; ri++) r.a[ri] = f(fo, wget(w,ri), xget(x,ri));
  else if (wr==0) { B c=wget(w, 0); for(; ri < ria; ri++) r.a[ri] = f(fo, inc(c)    , xget(x,ri)); dec(c); }
  else if (xr==0) { B c=xget(x, 0); for(; ri < ria; ri++) r.a[ri] = f(fo, wget(w,ri), inc(c)    ); dec(c); }
  else if (ria>0) {
    usz min = wg? a(x)->ia : a(w)->ia;
    usz ext = ria / min;
    if (wg) for (usz i = 0; i < min; i++) { B c=xget(x,i); for (usz j = 0; j < ext; j++,ri++) r.a[ri] = f(fo, wget(w,ri), inc(c)); }
    else    for (usz i = 0; i < min; i++) { B c=wget(w,i); for (usz j = 0; j < ext; j++,ri++) r.a[ri] = f(fo, inc(c), xget(x,ri)); }
  }
  B rb = harr_fc(r, bo);
  dec(w); dec(x);
  return rb;
}
B eachm_fn(BB2B f, B fo, B x) { // consumes x; x must be array
  usz ia = a(x)->ia;
  if (ia==0) return x;
  BS2B xget = TI(x).get;
  usz i = 0;
  B cr = f(fo, xget(x,0));
  HArr_p rH;
  if (TI(x).canStore(cr)) {
    bool reuse = reusable(x);
    if (v(x)->type==t_harr) {
      B* xp = harr_ptr(x);
      if (reuse) {
        dec(xp[i]); xp[i++] = cr;
        for (; i < ia; i++) xp[i] = f(fo, mv(xp,i));
        return x;
      } else {
        rH = m_harrs(ia, &i);
        rH.a[i++] = cr;
        for (; i < ia; i++) rH.a[i] = f(fo, inc(xp[i]));
        return harr_fcd(rH, x);
      }
    } else if (v(x)->type==t_i32arr) {
      i32* xp = i32arr_ptr(x);
      B r = reuse? x : m_i32arrc(x);
      i32* rp = i32arr_ptr(r);
      rp[i++] = o2iu(cr);
      for (; i < ia; i++) {
        cr = f(fo, m_i32(xp[i]));
        if (!q_i32(cr)) {
          rH = m_harrs(ia, &i);
          for (usz j = 0; j < i; j++) rH.a[j] = m_i32(rp[j]);
          if (!reuse) dec(r);
          goto fallback;
        }
        rp[i] = o2iu(cr);
      }
      if (!reuse) dec(x);
      return r;
    } else if (v(x)->type==t_f64arr) {
      f64* xp = f64arr_ptr(x);
      B r = reuse? x : m_f64arrc(x);
      f64* rp = f64arr_ptr(r);
      rp[i++] = o2iu(cr);
      for (; i < ia; i++) {
        cr = f(fo, m_f64(xp[i]));
        if (!q_f64(cr)) {
          rH = m_harrs(ia, &i);
          for (usz j = 0; j < i; j++) rH.a[j] = m_f64(rp[j]);
          if (!reuse) dec(r);
          goto fallback;
        }
        rp[i] = o2iu(cr);
      }
      if (!reuse) dec(x);
      return r;
    } else if (v(x)->type==t_fillarr) {
      B* xp = fillarr_ptr(x);
      if (reuse) {
        dec(c(FillArr,x)->fill);
        c(FillArr,x)->fill = bi_noFill;
        dec(xp[i]); xp[i++] = cr;
        for (; i < ia; i++) xp[i] = f(fo, mv(xp,i));
        return x;
      } else {
        HArr_p rp = m_harrs(ia, &i);
        rp.a[i++] = cr;
        for (; i < ia; i++) rp.a[i] = f(fo, inc(xp[i]));
        return harr_fcd(rp, x);
      }
    } else
    rH = m_harrs(ia, &i);
  } else
  rH = m_harrs(ia, &i);
  fallback:
  rH.a[i++] = cr;
  for (; i < ia; i++) rH.a[i] = f(fo, xget(x,i));
  return harr_fcd(rH, x);
}
B eachm(B f, B x) { // complete F¨ x
  if (!isArr(x)) return m_hunit(c1(f, x));
  if (isFun(f)) return eachm_fn(c(Fun,f)->c1, f, x);
  if (isMd(f)) if (!isArr(x) || a(x)->ia) { decR(x); thrM("Calling a modifier"); }
  
  usz ia = a(x)->ia;
  dec(x);
  HArr_p r = m_harrUv(ia);
  for(usz i = 0; i < ia; i++) r.a[i] = inc(f);
  return r.b;
}

B eachd(B f, B w, B x) { // complete w F¨ x
  if (!isArr(w) & !isArr(x)) return m_hunit(c2(f, w, x));
  if (isFun(f)) return eachd_fn(c(Fun,f)->c2, f, w, x);
  if (isArr(w) && isArr(x)) {
    ur mr = rnk(w); if(rnk(w)<mr) mr = rnk(w);
    if(!eqShPrefix(a(w)->sh, a(x)->sh, mr)) { decR(x); thrM("Mapping: Expected equal shape prefix"); }
  }
  if (isMd(f)) if ((isArr(w)&&a(w)->ia) || (isArr(x)&&a(x)->ia)) { decR(x); thrM("Calling a modifier"); } // case where both are units has already been taken care of
  
  HArr_p r = m_harrUc(!isArr(w)? x : rnk(w)>rnk(x)? w : x);
  for(usz i = 0; i < r.c->ia; i++) r.a[i] = inc(f);
  dec(w); dec(x);
  return r.b;
}
B shape_c1(B t, B x) {
  if (!isArr(x)) thrM("reshaping non-array");
  usz ia = a(x)->ia;
  if (reusable(x)) {
    decSh(x);
    arr_shVec(x, ia);
    return x;
  }
  B r = TI(x).slice(x, 0);
  arr_shVec(r, ia);
  return r;
}
B shape_c2(B t, B w, B x) {
  if (!isArr(x)) { dec(x); dec(w); thrM("reshaping non-array"); }
  if (!isArr(w)) return shape_c1(t, x);
  BS2B wget = TI(w).get;
  ur nr = a(w)->ia;
  usz nia = a(x)->ia;
  B r;
  if (reusable(x)) { r = x; decSh(x); }
  else r = TI(x).slice(x, 0);
  usz* sh = arr_shAllocI(r, nia, nr);
  if (sh) for (i32 i = 0; i < nr; i++) sh[i] = o2s(wget(w,i));
  dec(w);
  return r;
}

B pick_c1(B t, B x) {
  if (!isArr(x)) return x;
  if (a(x)->ia==0) {
    B r = getFill(x);
    if (noFill(r)) thrM("⊑: called on empty array without fill");
    return r;
  }
  B r = TI(x).get(x, 0);
  dec(x);
  return r;
}
B pick_c2(B t, B w, B x) {
  // usz wu = o2s(w);
  // if (!isArr(x)) { dec(x); dec(w); thrM("⊑: 𝕩 wasn't an array"); }
  // if (wu >= a(x)->ia) thrM("⊑: 𝕨 is greater than length of 𝕩"); // no bounds check for now
  B r = TI(x).get(x, o2su(w));
  dec(x);
  return r;
}

B ud_c1(B t, B x) {
  usz xu = o2s(x);
  if (xu<I32_MAX) {
    B r = m_i32arrv(xu); i32* rp = i32arr_ptr(r);
    for (usz i = 0; i < xu; i++) rp[i] = i;
    return r;
  }
  B r = m_f64arrv(xu); f64* rp = f64arr_ptr(r);
  for (usz i = 0; i < xu; i++) rp[i] = i;
  return r;
}

B pair_c1(B t,      B x) { return m_v1(   x); }
B pair_c2(B t, B w, B x) { return m_v2(w, x); }
B ltack_c1(B t,      B x) {         return x; }
B ltack_c2(B t, B w, B x) { dec(x); return w; }
B rtack_c1(B t,      B x) {         return x; }
B rtack_c2(B t, B w, B x) { dec(w); return x; }

B fmtN_c1(B t, B x) {
  const u64 BL = 100;
  char buf[BL];
  if (isF64(x)) snprintf(buf, BL, "%g", x.f);
  else snprintf(buf, BL, "(fmtN: not given a number?)");
  return m_str8(strlen(buf), buf);
}
B fmtF_c1(B t, B x) {
  if (!isVal(x)) return m_str32(U"(fmtF: not given a function)");
  u8 fl = v(x)->flags;
  if (fl==0 || fl>rtLen) return m_str32(U"(fmtF: not given a runtime primitive)");
  dec(x);
  return m_c32(U"+-×÷⋆√⌊⌈|¬∧∨<>≠=≤≥≡≢⊣⊢⥊∾≍↑↓↕«»⌽⍉/⍋⍒⊏⊑⊐⊒∊⍷⊔!˙˜˘¨⌜⁼´˝`∘○⊸⟜⌾⊘◶⎉⚇⍟⎊"[fl-1]);
}

B fne_c1(B t, B x) {
  if (isArr(x)) {
    ur xr = rnk(x);
    usz* sh = a(x)->sh;
    for (i32 i = 0; i < xr; i++) if (sh[i]>I32_MAX) {
      B r = m_f64arrv(xr); f64* rp = f64arr_ptr(r);
      for (i32 j = 0; j < xr; j++) rp[j] = sh[j];
      dec(x);
      return r;
    }
    B r = m_i32arrv(xr); i32* rp = i32arr_ptr(r);
    for (i32 i = 0; i < xr; i++) rp[i] = sh[i];
    dec(x);
    return r;
  } else {
    dec(x);
    return m_i32arrv(0);
  }
}
u64 depth(B x) { // doesn't consume
  if (!isArr(x)) return 0;
  if (TI(x).arrD1) return 1;
  u64 r = 0;
  usz ia = a(x)->ia;
  BS2B xgetU = TI(x).getU;
  for (usz i = 0; i < ia; i++) {
    u64 n = depth(xgetU(x,i));
    if (n>r) r = n;
  }
  return r+1;
}
B feq_c1(B t, B x) {
  u64 r = depth(x);
  dec(x);
  return m_f64(r);
}


B feq_c2(B t, B w, B x) {
  bool r = equal(w, x);
  dec(w); dec(x);
  return m_i32(r);
}
B fne_c2(B t, B w, B x) {
  bool r = !equal(w, x);
  dec(w); dec(x);
  return m_i32(r);
}

B funBI_identity(B x) {
  return inc(c(BFn,x)->ident);
}

B rt_select;
B select_c1(B t, B x) {
  if (!isArr(x)) thrM("⊏: Argument cannot be an atom");
  ur xr = rnk(x);
  if (xr==0) thrM("⊏: Argument cannot be rank 0");
  if (a(x)->sh[0]==0) thrM("⊏: Argument shape cannot start with 0");
  inc(x);
  B r = TI(x).slice(x,0);
  usz* sh = arr_shAllocR(r, xr-1);
  usz ia = 1;
  for (i32 i = 1; i < xr; i++) {
    if (sh) sh[i-1] = a(x)->sh[i];
    ia*= a(x)->sh[i];
  }
  a(r)->ia = ia;
  dec(x);
  return r;
}
B select_c2(B t, B w, B x) {
  if (isArr(w) && isArr(x)) {
    B xf = getFill(inc(x));
    BS2B wgetU = TI(w).getU;
    BS2B xget = TI(x).get;
    if (rnk(x)==1) {
      usz wia = a(w)->ia;
      usz xia = a(x)->ia;
      HArr_p r = m_harrUc(w);
      for (usz i = 0; i < wia; i++) {
        B cw = wgetU(w, i);
        if (!isNum(cw)) { harr_pfree(r.b, i); goto base; }
        f64 c = o2f(cw);
        if (c<0) c+= xia;
        if ((usz)c >= xia) thrM("⊏: Indexing out-of-bounds");
        r.a[i] = xget(x, c);
      }
      dec(w); dec(x);
      return withFill(r.b,xf);
    } else {
      ur wr = rnk(w); usz wia = a(w)->ia;
      ur xr = rnk(x);
      u32 rr = wr+xr-1;
      if (xr==0) thrM("⊏: 𝕩 cannot be a unit");
      if (rr>UR_MAX) thrM("⊏: Result rank too large");
      usz csz = arr_csz(x);
      usz cam = a(x)->sh[0];
      usz ria = wia*csz;
      HArr_p r = m_harrUp(ria);
      usz* rsh = arr_shAllocR(r.b, rr);
      if (rsh) {
        memcpy(rsh   , a(w)->sh  ,  wr   *sizeof(usz));
        memcpy(rsh+wr, a(x)->sh+1, (xr-1)*sizeof(usz));
      }
      for (usz i = 0; i < wia; i++) {
        B cw = wgetU(w, i);
        if (!isNum(cw)) { harr_pfree(r.b, i); goto base; }
        f64 c = o2f(cw);
        if (c<0) c+= cam;
        if ((usz)c >= cam) thrM("⊏: Indexing out-of-bounds");
        for (usz j = 0; j < csz; j++) r.a[i*csz+j] = xget(x, c*csz+j);
      }
      dec(w); dec(x);
      return withFill(r.b,xf);
    }
  }
  base:
  return c2(rt_select, w, x);
}

i64 isum(B x) { // doesn't consume; assumes is array; may error
  BS2B xgetU = TI(x).getU;
  i64 r = 0;
  usz xia = a(x)->ia;
  for (usz i = 0; i < xia; i++) r+= o2f(xgetU(x,i)); // TODO error on overflow and non-integers or something
  return r;
}

B rt_slash;
B slash_c1(B t, B x) {
  if (!isArr(x)) thrM("/: Argument must be a list");
  if (rnk(x)!=1) thrM("/: Argument must have rank 1");
  i64 s = isum(x);
  if(s<0) thrM("/: Argument must consist of natural numbers");
  usz xia = a(x)->ia;
  BS2B xgetU = TI(x).getU;
  usz ri = 0;
  if (xia<I32_MAX) {
    B r = m_i32arrv(s); i32* rp = i32arr_ptr(r);
    for (usz i = 0; i < xia; i++) {
      usz c = o2s(xgetU(x, i));
      for (usz j = 0; j < c; j++) rp[ri++] = i;
    }
    dec(x);
    return r;
  }
  HArr_p r = m_harrs(s, &ri);
  for (usz i = 0; i < xia; i++) {
    usz c = o2s(xgetU(x, i));
    for (usz j = 0; j < c; j++) r.a[ri++] = m_i32(i);
  }
  dec(x);
  return withFill(harr_fv(r),m_f64(0));
}
B slash_c2(B t, B w, B x) {
  if (isArr(w) && isArr(x) && rnk(w)==1 && rnk(x)==1 && depth(w)==1) {
    usz wia = a(w)->ia;
    usz xia = a(x)->ia;
    B xf = getFill(inc(x));
    if (wia!=xia) thrM("/: Lengths of components of 𝕨 must match 𝕩");
    usz ria = isum(w);
    usz ri = 0;
    HArr_p r = m_harrs(ria, &ri);
    BS2B wgetU = TI(w).getU;
    BS2B xgetU = TI(x).getU;
    for (usz i = 0; i < wia; i++) {
      B cw = wgetU(w, i);
      if (isNum(cw)) {
        f64 cf = o2f(cw);
        usz c = (usz)cf;
        if (cf!=c) goto base; // TODO clean up half-written r
        if (c) {
          B cx = xgetU(x, i);
          for (usz j = 0; j < c; j++) r.a[ri++] = inc(cx);
        }
      } else { dec(cw); goto base; }
    }
    dec(w); dec(x);
    return withFill(harr_fv(r), xf);
  }
  base:
  return c2(rt_slash, w, x);
}

B slicev(B x, usz s, usz ia) {
  usz xia = a(x)->ia; if (s+ia>xia) thrM("↑/↓: NYI fills");
  B r = TI(x).slice(x, s);
  arr_shVec(r, ia);
  return r;
}
B take_c2(B t, B w, B x) {
  if (!isArr(x) || rnk(x)!=1) thrM("↑: NYI 1≠=𝕩");
  i64 v = o2i64(w); usz ia = a(x)->ia;
  return v<0? slicev(x, ia+v, -v) : slicev(x, 0, v);
}
B drop_c2(B t, B w, B x) {
  if (!isArr(x) || rnk(x)!=1) thrM("↓: NYI 1≠=𝕩");
  i64 v = o2i64(w); usz ia = a(x)->ia;
  return v<0? slicev(x, 0, v+ia) : slicev(x, v, ia-v);
}
B join_c2(B t, B w, B x) {
  if (!isArr(w)|!isArr(x) || rnk(w)!=1 | rnk(x)!=1) thrM("∾: NYI non-vector args");
  usz wia = a(w)->ia; BS2B wget = TI(w).get;
  usz xia = a(x)->ia; BS2B xget = TI(x).get;
  HArr_p r = m_harrUv(wia+xia);
  for (i64 i = 0; i < wia; i++) r.a[i    ] = wget(w, i);
  for (i64 i = 0; i < xia; i++) r.a[i+wia] = xget(x, i);
  dec(x); dec(w);
  return r.b;
}

#define ba(N) bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); c(Fun,bi_##N)->c2 = N##_c2    ;c(Fun,bi_##N)->c1 = N##_c1    ; c(Fun,bi_##N)->extra=pf_##N; c(BFn,bi_##N)->ident=bi_N; gc_add(bi_##N);
#define bd(N) bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); c(Fun,bi_##N)->c2 = N##_c2    ;c(Fun,bi_##N)->c1 = c1_invalid; c(Fun,bi_##N)->extra=pf_##N; c(BFn,bi_##N)->ident=bi_N; gc_add(bi_##N);
#define bm(N) bi_##N = mm_alloc(sizeof(BFn), t_funBI, ftag(FUN_TAG)); c(Fun,bi_##N)->c2 = c2_invalid;c(Fun,bi_##N)->c1 = N##_c1    ; c(Fun,bi_##N)->extra=pf_##N; c(BFn,bi_##N)->ident=bi_N; gc_add(bi_##N);

void print_fun_def(B x) { printf("%s", format_pf(c(Fun,x)->extra)); }

B                                bi_shape, bi_pick, bi_ud, bi_pair, bi_fne, bi_feq, bi_select, bi_slash, bi_ltack, bi_rtack, bi_join, bi_take, bi_drop, bi_fmtF, bi_fmtN;
static inline void sfns_init() { ba(shape) ba(pick) bm(ud) ba(pair) ba(fne) ba(feq) ba(select) ba(slash) ba(ltack) ba(rtack) bd(join) bd(take) bd(drop) bm(fmtF) bm(fmtN)
  ti[t_funBI].print = print_fun_def;
  ti[t_funBI].identity = funBI_identity;
}

#undef ba
#undef bd
#undef bm
