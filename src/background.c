#include <gpu.h>
#include <layer.h>

typedef struct /*4 per Layer*/
{
    DR_TPAGE tpages[8 + 4]; //8 * 4bpp , 4 * 8bpp
} LayerPages;
/*Needs 0xB40 bytes*/

extern LayerPages layerTpages[2][3][2]; /*Buffer | Layer | Priority*/

static int rectCount = 0;

void SetupTpages();
void DrawBackgroundLayer(int layer,ushort baseClut,u_char * layoutP);

void DrawBackground()
{
    rectCount = 0;

    SetupTpages();

    if (bgLayers[0].display)
    {
        ushort baseClut;
        if ((fadeFlags & 1) == 0)
        {
            baseClut = 0x7900;
        }else{
            baseClut = 0x7B00;
        }
        DrawBackgroundLayer(0,baseClut,&screenLayout);
    }
    if (bgLayers[1].display)
    {
        ushort baseClut;
        if ((fadeFlags & 2) == 0)
        {
            baseClut = 0x7900;
        }else{
            baseClut = 0x7B00;
        }
        DrawBackgroundLayer(1,baseClut,&screenLayout2);
    }
    if (bgLayers[2].display)
    {
        ushort baseClut;
        if ((fadeFlags & 4) == 0)
        {
            baseClut = 0x7900;
        }else{
            baseClut = 0x7B00;
        }
        DrawBackgroundLayer(2,baseClut,&screenLayout3);
    }
}

void DrawBackgroundLayer(int layer,ushort baseClut,u_char * layoutP)
{
    u_long *ot,otP;

    /*Setup Priority & Link Tpage commands*/
    if (bgLayers[layer].priority == 1)
    {
        ot = &drawP->ot[2];
        otP = &drawP->ot[4];
    }
    else if (bgLayers[layer].priority < 2)
    {
        if (bgLayers[layer].priority == 0)
        {
            ot = &drawP->ot[0];
            otP = ot;
        }
    }
    else if (bgLayers[layer].priority == 2)
    {
        ot = &drawP->ot[5];
        otP = &drawP->ot[7];
    }
    AddPrims(ot,&layerTpages[buffer][layer][0].tpages[0],&layerTpages[buffer][layer][0].tpages[8 + 4 - 1]);
    AddPrims(otP,&layerTpages[buffer][layer][1].tpages[0],&layerTpages[buffer][layer][1].tpages[8 + 4 - 1]);

    int innerX = bgLayers[layer].x & 0xF;
    int innerY = bgLayers[layer].y & 0xF;

    for (size_t y = 0; y < 16; y++)
    {
        for (size_t x = 0; x < 21; x++)
        {
            int x16 = bgLayers[layer].x + x * 16;
            int y16 = bgLayers[layer].y + y * 16;
            int screenId = layoutP[(x16 >> 8) + (y16 >> 8) * 32];
            ushort tileVal = ((ushort*)0x80171c3c)[screenId * 0x100 + ((x16 & 0xF0) >> 4) + (y16 & 0xF0)];

            u_char cordX = (((u_char*)0x8015ea88)[(tileVal & 0xFFF) * 4] & 0xF) * 16;
            u_char cordY = ((u_char*)0x8015ea88)[(tileVal & 0xFFF) * 4] & 0xF0;
            u_char page = ((u_char*)0x8015ea88)[((tileVal & 0xFFF) * 4) + 1];
            u_char clut = ((u_char*)0x8015ea88)[((tileVal & 0xFFF) * 4) + 2];
            if (page >= 4 + 8 || (tileVal & 0xFFF) == 0)
            {
                continue;
            }
            

            //16x16 Flat Shadded Rectangle
            SPRT_16 * p = &rectBuffer[buffer][rectCount];
            setSprt16(p);
            if ((tileVal & 0x2000) != 0)
            {
                setSemiTrans(p,1);
            }
            setRGB0(p,0x80,0x80,0x80);
            setUV0(p,cordX,cordY);
            setXY0(p,(x * 16) - innerX,(y * 16) - innerY);
            p->clut = getClut((clut * 16) & 0xFF,clut >> 4) + baseClut;

            int pri = 0;
            if ((tileVal & 0x1000) != 0)
            {
                pri = 1;
            }
            

            //Add to Texture Page Command
            AddPrim(&layerTpages[buffer][layer][pri].tpages[page],p);
            rectCount++;
        }
    }
    ///////////////////////
    
}
void SetupTpages()
{
    for (size_t l = 0; l < 3; l++)
    {
        for (size_t p = 0; p < (8 + 4); p++)
        {
            if (p < 8)
            {
                SetDrawTPage(&layerTpages[buffer][l][0].tpages[p], 0,0, GetTPageValue(0,1,8 + p,1));
                CatPrim(&layerTpages[buffer][l][0].tpages[p],&layerTpages[buffer][l][0].tpages[p + 1]);
                SetDrawTPage(&layerTpages[buffer][l][1].tpages[p], 0,0, GetTPageValue(0,1,8 + p,1));
                CatPrim(&layerTpages[buffer][l][1].tpages[p],&layerTpages[buffer][l][1].tpages[p + 1]);
            }
            else
            {
                SetDrawTPage(&layerTpages[buffer][l][0].tpages[p], 0,0, GetTPageValue(1,1,8 + (p - 8) * 2,1));
                CatPrim(&layerTpages[buffer][l][0].tpages[p],&layerTpages[buffer][l][0].tpages[p + 1]);
                SetDrawTPage(&layerTpages[buffer][l][1].tpages[p], 0,0, GetTPageValue(1,1,8 + (p - 8) * 2,1));
                CatPrim(&layerTpages[buffer][l][1].tpages[p],&layerTpages[buffer][l][1].tpages[p + 1]);
            }
            
        }
        
    }
       
}