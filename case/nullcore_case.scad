// ============================================================
//  nullcore — корпус (tray), открытый сзади.
//  Экран ST7789 2.0" спереди, 3 кнопки под экраном, вырез под USB-C.
//  Все размеры в мм — ПОДГОНИ под свои детали (замерь штангелем).
// ============================================================
$fn = 72;

// ===== РЕДАКТИРУЙ ЭТИ РАЗМЕРЫ (мм) =====
wall        = 2.4;   // толщина стенок
depth       = 17;    // внутренняя глубина (стопка электроники)
clr         = 1.5;   // зазор вокруг платы экрана с каждой стороны

disp_pcb_w  = 37.5;  // ширина ПЛАТЫ модуля экрана
win_w       = 31;    // ширина видимого окна экрана
win_h       = 41;    // высота видимого окна экрана
win_top     = 9;     // отступ окна от верхнего внутреннего края

btn_dia     = 9;     // диаметр отверстия под кнопку/колпачок
btn_n       = 3;     // число кнопок
btn_pitch   = 14;    // расстояние между центрами кнопок
btn_row_gap = 13;    // зазор от низа окна до ряда кнопок

usb_w       = 11;    // ширина выреза USB-C
usb_h       = 5.5;   // высота выреза USB-C
usb_z       = 8;     // центр выреза USB по глубине (от передней стенки)

corner_r    = 4;     // радиус внешних углов
// =======================================

inner_w = disp_pcb_w + 2*clr;
inner_h = win_top + win_h + btn_row_gap + 16;
outer_w = inner_w + 2*wall;
outer_h = inner_h + 2*wall;
outer_d = depth + wall;

module rbox(w,h,d,r){
    hull() for(x=[r, w-r]) for(y=[r, h-r])
        translate([x,y,0]) cylinder(r=r, h=d);
}

difference(){
    // корпус
    rbox(outer_w, outer_h, outer_d, corner_r);

    // полость (открыта сзади, +Z); передняя стенка остаётся толщиной wall
    translate([wall, wall, wall])
        cube([inner_w, inner_h, outer_d]);

    // окно экрана (режем переднюю стенку), по центру по X, ближе к верху по Y
    translate([ (outer_w-win_w)/2, outer_h - wall - win_top - win_h, -1])
        cube([win_w, win_h, wall+2]);

    // ряд кнопок под окном
    by = outer_h - wall - win_top - win_h - btn_row_gap;
    for(i=[0:btn_n-1])
        translate([ outer_w/2 + (i-(btn_n-1)/2)*btn_pitch, by, -1])
            cylinder(d=btn_dia, h=wall+2);

    // вырез под USB-C в нижней боковой стенке (Y=0)
    translate([ (outer_w-usb_w)/2, -1, usb_z - usb_h/2])
        cube([usb_w, wall+2, usb_h]);
}
