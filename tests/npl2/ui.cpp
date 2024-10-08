//nya-engine (C) nyan.developer@gmail.com released under the MIT license (see LICENSE)

#include "ui.h"

#include "render/render.h"

#include "attributes.h"
#include "scene.h"

#include "string.h"

void ui::init()
{
    widget_renderer::get().init();
    
    const int btn_width=72;
    const int btn_height=22;
    const int offset=6;

    int xpos=offset;

    button customize_btn;
    customize_btn.set_id("customize_btn");
    customize_btn.set_text("Customize");
    customize_btn.set_pos(xpos,offset);
    customize_btn.set_size(btn_width,btn_height);
    add_widget(customize_btn);

    xpos+=btn_width+offset;

    button anim_btn;
    anim_btn.set_id("anim_btn");
    anim_btn.set_text("Anims");
    anim_btn.set_pos(xpos,offset);
    anim_btn.set_size(btn_width,btn_height);
    add_widget(anim_btn);

    xpos+=btn_width+offset;

    button scenery_btn;
    scenery_btn.set_id("scenery_btn");
    scenery_btn.set_text("Scenery");
    scenery_btn.set_pos(xpos,offset);
    scenery_btn.set_size(btn_width,btn_height);
    add_widget(scenery_btn);

    button options_btn;
    options_btn.set_id("options_btn");
    options_btn.set_text("Options");
    options_btn.set_align(false,true,false,true);
    options_btn.set_pos(get_width()-btn_width-offset,offset);
    options_btn.set_size(btn_width,btn_height);
    add_widget(options_btn);

    const int panel_width=228;
    const int panel_pos_y=2*offset+btn_height;

    unsigned int panel_height=get_height()-panel_pos_y-offset;
    
    const uint buttons_pnl_h=(btn_height+offset)*4;

    m_customise_lst.create();
    get_attribute_manager().reset_iterator();
    const char *atr=get_attribute_manager().iterate_next();
    if(atr)
    {
        m_customise_lst->add_element("none");

        get_attribute_manager().iterate_elements(atr);
        const char *elem=get_attribute_manager().iterate_next_element();
        while(elem)
        {
            m_customise_lst->add_element(elem);

            elem=get_attribute_manager().iterate_next_element();
        }

        m_customise_group.assign(atr);

    }

    //const char *def_group=atr;

    m_customize_pnl.create();
    m_customize_pnl.set_id("customize_pnl");
    m_customize_pnl.set_align(true,false,true,true);
    m_customize_pnl.set_pos(offset,panel_pos_y);
    m_customize_pnl.set_size(panel_width,panel_height);
    m_customize_pnl.set_visible(false);

    while(atr)
    {
        if(strcmp(atr,"BG")==0)
        {
            atr=get_attribute_manager().iterate_next();
            continue;
        }

        button btn;
        btn.set_id(atr);
        if(strcmp(atr,"COORDINATE")==0)
            btn.set_text("SET");
        else
            btn.set_text(atr);

        const int idx=int(m_customize_groups.size());

        uint x=idx%3;
        uint y=idx/3;

        uint cust_btn_width=btn_width-4;

        btn.set_pos(offset+(cust_btn_width+offset)*x,buttons_pnl_h+offset-(btn_height+offset)*(y+1));
        btn.set_size(cust_btn_width,btn_height);
        m_customize_pnl->add_widget(btn);

        m_customize_groups.push_back(atr);

        atr=get_attribute_manager().iterate_next();
    }

    m_customise_lst.set_align(true,false,true,true);
    m_customise_lst.set_id("customise_lst");
    m_customise_lst.set_pos(offset,offset+buttons_pnl_h);
    m_customise_lst.set_size(panel_width-offset*2,panel_height-offset*2-buttons_pnl_h);
    m_customize_pnl->add_widget_proxy(m_customise_lst);
    add_widget_proxy(m_customize_pnl);

    panel::style modal_bg;
    modal_bg.panel.border=false;
    modal_bg.panel.solid=true;
    modal_bg.panel.solid_color.set(0,0,0,0.7);

    panel cos_modal;
    cos_modal.set_id("cos_modal");
    cos_modal.set_align(true,false,true,true);
    cos_modal.set_pos(offset,panel_pos_y);
    cos_modal.set_size(panel_width,panel_height);
    cos_modal.set_style(modal_bg);
    cos_modal.set_visible(false);

    const int modal_box_width=btn_width+offset*2;
    const int modal_box_height=btn_height*3+offset*4;

    panel::style mod_box_bg;
    mod_box_bg.panel.solid=true;
    mod_box_bg.panel.solid_color=mod_box_bg.panel.border_color;
    mod_box_bg.panel.solid_color.a=0.3;

    panel mod_box;
    mod_box.set_id("mod_box");
    mod_box.set_pos(20,40);
    mod_box.set_style(mod_box_bg);
    mod_box.set_size(modal_box_width,modal_box_height);
    mod_box.set_visible(false);

    const char *modal_captions[]={"BOTH","BOTTOM","TOP"};
    for(int i=0;i<sizeof(modal_captions)/sizeof(modal_captions[0]);++i)
    {
        button btn;
        btn.set_pos(offset,(offset+btn_height)*i+offset);
        btn.set_size(btn_width,btn_height);
        btn.set_text(modal_captions[i]);

        std::string id("cos_mod_btn");
        id.push_back('0'+i);
        btn.set_id(id.c_str());
        mod_box.add_widget(btn);
    }

    add_widget(cos_modal);
    add_widget(mod_box);

    const int props_width=200;
    const int props_height=100;
    panel props_pnl;
    props_pnl.set_id("props_pnl");
    props_pnl.set_align(false,true,true,false);
    props_pnl.set_pos(get_width()-offset-props_width,get_height()-props_height-offset);
    props_pnl.set_size(props_width,props_height);
    props_pnl.set_visible(false);

    label opac_lbl;
    opac_lbl.set_text("Opacity");
    opac_lbl.set_pos(0,props_height-btn_height-offset);
    opac_lbl.set_size(btn_width,btn_height);
    props_pnl.add_widget(opac_lbl);

    button opac_reset_btn;
    opac_reset_btn.set_id("opac_reset_btn");
    opac_reset_btn.set_text("Reset all");
    opac_reset_btn.set_pos(props_width-(btn_width+offset),props_height-(btn_height+offset));
    opac_reset_btn.set_size(btn_width,btn_height);
    props_pnl.add_widget(opac_reset_btn);

    const int slider_height=10;
    const int slider_width=props_width-offset*2;
    const int slider_y=props_height-btn_height-slider_height-offset*2;
    m_opac_slider.create();
    m_opac_slider.set_id("opac_sldr");
    m_opac_slider.set_pos(offset,slider_y);
    m_opac_slider.set_size(slider_width,slider_height);
    props_pnl.add_widget_proxy(m_opac_slider);


    panel::style under_pnl_style;
    under_pnl_style.panel.border=false;
    const int under_pnl_height=slider_y;

    panel under_pnl;
    under_pnl.set_id("under_pnl");
    under_pnl.set_pos(0,0);
    under_pnl.set_size(props_width,under_pnl_height);
    under_pnl.set_style(under_pnl_style);
    under_pnl.set_visible(false);

    label under_lbl;
    under_lbl.set_text("Under toggle");
    under_lbl.set_pos(0,btn_height+offset*1.5);
    under_lbl.set_size(btn_width*1.5,btn_height);
    under_pnl.add_widget(under_lbl);

    button under_top;
    under_top.set_id("under_top");
    under_top.set_text("Top");
    under_top.set_pos(offset,offset);
    under_top.set_size(btn_width,btn_height);
    under_pnl.add_widget(under_top);

    button under_btm;
    under_btm.set_id("under_btm");
    under_btm.set_text("Bottom");
    under_btm.set_pos(offset*2+btn_width,offset);
    under_btm.set_size(btn_width,btn_height);
    under_pnl.add_widget(under_btm);

    props_pnl.add_widget(under_pnl);

    add_widget(props_pnl);

    //if(m_imouto)
    //    m_customise_lst.select_element(m_imouto->get_attrib(def_group));

    panel anim_pnl;
    anim_pnl.set_id("anim_pnl");
    anim_pnl.set_align(true,false,true,true);
    anim_pnl.set_pos(offset,panel_pos_y);
    anim_pnl.set_size(panel_width,panel_height);
    anim_pnl.set_visible(false);

    m_anim_lst.create();
    m_anim_lst.set_id("anim_lst");
    m_anim_lst.set_align(true,false,true,true);
    m_anim_lst.set_pos(offset,offset);
    m_anim_lst.set_size(panel_width-offset*2,panel_height-offset*2);

    for(uint i=0;i<get_scene().get_anims_count();++i)
        m_anim_lst->add_element(get_scene().get_anim_name(i));

    anim_pnl.add_widget_proxy(m_anim_lst);

    add_widget(anim_pnl);

    panel scenery_pnl;
    scenery_pnl.set_id("scenery_pnl");
    scenery_pnl.set_align(true,false,true,true);
    scenery_pnl.set_pos(offset,panel_pos_y);
    scenery_pnl.set_size(panel_width,panel_height);
    scenery_pnl.set_visible(false);

    m_scenery_lst.create();
    m_scenery_lst.set_align(true,false,true,true);
    m_scenery_lst.set_id("scenery_lst");
    m_scenery_lst.set_pos(offset,offset);
    m_scenery_lst.set_size(panel_width-offset*2,panel_height-offset*2);

    m_scenery_lst->add_element("none");
    get_attribute_manager().iterate_elements("BG");
    while(const char *atr=get_attribute_manager().iterate_next_element())
        m_scenery_lst->add_element(atr);

    scenery_pnl.add_widget_proxy(m_scenery_lst);
    add_widget(scenery_pnl);
}

void ui::draw()
{
    nya_render::blend::enable(nya_render::blend::src_alpha,nya_render::blend::inv_src_alpha);
    nya_render::depth_test::disable();

    nya_ui::layer::draw();

    nya_render::blend::disable();
}

bool ui::is_props_visible()
{
    if(!get_widget("customize_pnl").is_visible())
        return false;

    if(m_customise_group=="BODY" || m_customise_group=="HAIR"
       || m_customise_group=="EYE" || m_customise_group=="COORDINATE")
        return false;

    return true;
}

void ui::update_props_panel()
{
    const bool visible=is_props_visible();

    get_widget("props_pnl").set_visible(visible);

    if(!visible)
        return;

    const bool under=(m_customise_group=="UNDER");
    get_widget("under_pnl").set_visible(under);

    int num=-1;
    if(m_customise_group=="COSTUME" || under)
    {
        if(m_custom_mode==cos_up)
            num=0;
        else if(m_custom_mode==cos_dn)
            num=1;
    }

    if(!m_opac_slider.is_valid())
        return;

    const float value=get_scene().get_part_opacity(m_customise_group.c_str(),num);
    m_opac_slider->set_value(value);
}

void ui::modal(bool enabled,int x,int y)
{
    get_widget("cos_modal").set_visible(enabled);
    get_widget("mod_box").set_visible(enabled);

    const int offset=6;

    int px,py;
    get_widget("cos_modal").get_pos(px,py);

    if(enabled)
        get_widget("mod_box").set_pos(x+px-offset,y+py-offset);
}

void ui::process_events(const nya_ui::event &e)
{
    if(e.sender=="customise_lst" && m_customise_lst.is_valid())
    {
        if(e.type=="select_element")
        {
            const char *selected=m_customise_lst->get_selected_element();
            if(!selected)
                return;

            if(m_custom_mode==cos_up)
                get_scene().set_imouto_attr(m_customise_group.c_str(),selected,0);
            else if(m_custom_mode==cos_dn)
                get_scene().set_imouto_attr(m_customise_group.c_str(),selected,1);
            else
                get_scene().set_imouto_attr(m_customise_group.c_str(),selected);

            return;
        }

        if(e.type=="mover_element")
        {
            const char *selected=m_customise_lst->get_mover_element();
            if(!selected)
                return;

            if(m_custom_mode==cos_up)
                get_scene().set_imouto_preview(m_customise_group.c_str(),selected,0);
            else if(m_custom_mode==cos_dn)
                get_scene().set_imouto_preview(m_customise_group.c_str(),selected,1);
            else
                get_scene().set_imouto_preview(m_customise_group.c_str(),selected);

            return;
        }

        if(e.type=="mleft_elements")
            get_scene().finish_imouto_preview();

        return;
    }

    if(e.sender=="scenery_lst" && e.type=="select_element" && m_scenery_lst.is_valid())
    {
        const char *selected=m_scenery_lst->get_selected_element();
        if(!selected)
            return;

        get_scene().set_bkg(selected);
        return;
    }

    if(e.sender=="anim_lst")
    {
        if(e.type=="select_element" && m_anim_lst.is_valid())
        {
            get_scene().set_anim(m_anim_lst->get_selected_idx());
            return;
        }
    }

    if(e.sender=="opac_sldr")
    {
        if(e.type=="value_changed" && m_opac_slider.is_valid())
        {
            int num=-1;
            if(m_customise_group=="COSTUME" || m_customise_group=="UNDER")
            {
                if(m_custom_mode==cos_up)
                    num=0;
                else if(m_custom_mode==cos_dn)
                    num=1;
            }

            get_scene().set_part_opacity(m_customise_group.c_str(),m_opac_slider->get_value(),num);
        }
        return;
    }

    if(e.type!="button_pressed")
        return;

    if(e.sender=="under_top")
    {
        m_under_top=!m_under_top;
        get_scene().set_imo_under_state(m_under_top,m_under_bottom);
        return;
    }

    if(e.sender=="under_btm")
    {
        m_under_bottom=!m_under_bottom;
        get_scene().set_imo_under_state(m_under_top,m_under_bottom);
        return;
    }

    if(e.sender=="cos_mod_btn0" && m_customise_lst.is_valid())
    {
        m_custom_mode=none;
        modal(false,0,0);
    
        const char *atr=get_scene().get_imouto_attr(m_customise_group.c_str());
        if(atr)
            m_customise_lst->select_element(atr);
        
        return;
    }

    if(e.sender=="cos_mod_btn1" && m_customise_lst.is_valid())
    {
        m_custom_mode=cos_dn;
        modal(false,0,0);
        
        const char *atr=get_scene().get_imouto_attr(m_customise_group.c_str(),1);
        if(atr)
            m_customise_lst->select_element(atr);
        
        return;
    }

    if(e.sender=="cos_mod_btn2" && m_customise_lst.is_valid())
    {
        m_custom_mode=cos_up;
        modal(false,0,0);

        const char *atr=get_scene().get_imouto_attr(m_customise_group.c_str(),0);
        if(atr)
            m_customise_lst->select_element(atr);
        
        return;
    }

    if(e.sender=="opac_reset_btn" && m_opac_slider.is_valid())
    {
        get_scene().reset_parts_opacity();
        m_opac_slider->set_value(1.0f);
        return;
    }

    if(e.sender=="customize_btn")
    {
        get_widget("customize_pnl").set_visible(!get_widget("customize_pnl").is_visible());
        update_props_panel();
        get_widget("anim_pnl").set_visible(false);
        get_widget("scenery_pnl").set_visible(false);
        modal(false,0,0);
        return;
    }

    if(e.sender=="anim_btn")
    {
        get_widget("customize_pnl").set_visible(false);
        update_props_panel();
        get_widget("anim_pnl").set_visible(!get_widget("anim_pnl").is_visible());
        get_widget("scenery_pnl").set_visible(false);
        modal(false,0,0);
        return;
    }

    if(e.sender=="scenery_btn")
    {
        get_widget("customize_pnl").set_visible(false);
        update_props_panel();
        get_widget("anim_pnl").set_visible(false);
        get_widget("scenery_pnl").set_visible(!get_widget("scenery_pnl").is_visible());
        modal(false,0,0);
        return;
    }

    if(m_customize_pnl.is_valid() && m_customise_lst.is_valid())
    for(int i=0;i<int(m_customize_groups.size());++i)
    {
        if(e.sender!=m_customize_groups[i])
            continue;

        m_custom_mode=none;

        m_customise_lst->remove_elements();
        if(e.sender!="BODY" && e.sender!="HAIR" && e.sender!="EYE" && e.sender!="COORDINATE")
            m_customise_lst->add_element("none");

        m_customise_group=e.sender;

        if(m_customise_group=="COSTUME" || m_customise_group=="UNDER")
        {
            for(int i=0;i<int(m_customize_groups.size());++i)
            {
                if(m_customize_groups[i]==m_customise_group)
                {
                    int x,y;
                    m_customize_pnl->get_widget(m_customize_groups[i].c_str()).get_pos(x,y);
                    modal(true,x,y);
                    break;
                }
            }
        }

        get_attribute_manager().iterate_elements(m_customise_group.c_str());

        const char *elem=get_attribute_manager().iterate_next_element();
        while(elem)
        {
            m_customise_lst->add_element(elem);

            elem=get_attribute_manager().iterate_next_element();
        }

        update_props_panel();

        int num=-1;
        if(m_custom_mode==cos_up)
            num=0;
        else if(m_custom_mode==cos_dn)
            num=1;

        const char *atr=get_scene().get_imouto_attr(m_customise_group.c_str(),num);
        if(atr)
            m_customise_lst->select_element(atr);

        return;
    }
}
