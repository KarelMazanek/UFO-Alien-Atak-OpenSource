#pragma once
#include <algorithm>
namespace uaa {
struct UiLayout {
 int width,height,sidebar,top,bottom=0,paletteTop,paletteHeight,columns=3;
 explicit UiLayout(int w,int h):width(w),height(h){sidebar=360;top=64;paletteTop=top+31;paletteHeight=std::max(1,h-bottom-paletteTop);}
 int cardWidth(int count=5)const{return (sidebar-16-(count-1)*3)/count;}
 int tabWidth()const{return (sidebar-12-5*3)/6;}
};
}
