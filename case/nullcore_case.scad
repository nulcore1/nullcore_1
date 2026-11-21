// ============================================================
//  nullcore — корпус (tray), открытый сзади. Скруглённый, «красивый».
//  Экран ST7789 2.0" (PCB 37.5 x 56.3 мм), 3 квадратные кнопки, USB-C.
//  Размеры параметрические — подгони штангелем.
// ============================================================
$fn = 64;

// ===== ЭКРАН (мм) — стандартный 2.0" ST7789 =====
disp_pcb_w  = 37.5;  // ширина платы экрана
disp_pcb_h  = 56.3;  // высота платы экрана
win_w       = 31;    // ширина видимого окна
win_h       = 41;    // высота видимого окна
win_r       = 3;     // скругление углов окна

// ===== КНОПКИ (квадратные) =====
btn_size    = 6;     // сторона квадратного отверстия (кнопка 5 мм + зазор)
btn_r       = 1.2;   // скругление углов кнопочного отверстия
btn_n       = 3;     // число кнопок
btn_pitch   = 14;    // расстояние между центрами
btn_zone    = 22;    // высота зоны кнопок ниже экрана

// ===== КОРПУС =====
wall        = 2.4;   // толщина стенок
depth       = 17;    // внутренняя глубина
clr         = 1.5;   // зазор вокруг платы экрана
edge_r      = 3.5;   // скругление ВСЕХ внешних рёбер

// ===== USB-C =====
usb_w       = 11;    // ширина выреза
usb_h       = 5.5;   // высота выреза
usb_z       = 8;     // центр выреза по глубине (от передней стенки)
// =================================================

inner_w = disp_pcb_w + 2*clr;
inner_h = disp_pcb_h + btn_zone;
outer_w = inner_w + 2*wall;
outer_h = inner_h + 2*wall;
outer_d = depth + wall;

win_y = wall + btn_zone + (disp_pcb_h - win_h)/2;
btn_y = wall + btn_zone/2;

// корпус со скруглением всех рёбер (плоские грани + мягкие рёбра)
module rshell(w,h,d,r){
    minkowski(){
        translate([r,r,r]) cube([w-2*r, h-2*r, d-2*r]);
        sphere(r, $fn=28);
    }
}

// скруглённый прямоугольный вырез (ось по Z)
module rrect_z(w,h,r,hh){
    hull() for(x=[r,w-r]) for(y=[r,h-r])
        translate([x,y,0]) cylinder(r=r, h=hh, $fn=40);
}

// скруглённый (стадион) вырез вдоль оси Y — для USB
module yslot(x,z,r,len){ translate([x,-1,z]) rotate([-90,0,0]) cylinder(r=r,h=len,$fn=36); }

difference(){
    rshell(outer_w, outer_h, outer_d, edge_r);

    // полость, открыта сзади
    translate([wall, wall, wall]) cube([inner_w, inner_h, outer_d+edge_r]);

    // окно экрана (скруглённые углы)
    translate([(outer_w-win_w)/2, win_y, -1]) rrect_z(win_w, win_h, win_r, wall+2);

    // квадратные кнопки со слегка скруглёнными углами
    for(i=[0:btn_n-1])
        translate([outer_w/2 + (i-(btn_n-1)/2)*btn_pitch - btn_size/2, btn_y - btn_size/2, -1])
            rrect_z(btn_size, btn_size, btn_r, wall+2);

    // USB-C — овальный вырез снизу
    ur = usb_h/2; len = wall+2;
    x0 = (outer_w-usb_w)/2 + ur; x1 = (outer_w+usb_w)/2 - ur;
    hull(){ yslot(x0, usb_z, ur, len); yslot(x1, usb_z, ur, len); }
}
