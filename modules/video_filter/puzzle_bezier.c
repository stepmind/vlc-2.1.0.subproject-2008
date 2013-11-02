/*****************************************************************************
 * puzzle_bezier.c : Bezier curves management
 *****************************************************************************
 * Copyright (C) 2013 Vianney Boyer
 * $Id: 92cc1788030d6bdc1064b483071602086e7da5be $
 *
 * Author:  Vianney Boyer <vlcvboyer -at- gmail -dot- com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#include "stdafx.h"

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include <math.h>

#include <vlc_common.h>
#include <vlc_rand.h>

#include "puzzle_bezier.h"

/*****************************************************************************
 * scale bezier curve in order to fit into piece sector (avoid overlapping)
 *****************************************************************************/
point_t *puzzle_scale_curve_H(int32_t i_width, int32_t i_lines, uint8_t i_pts_nbr, point_t *ps_pt, int32_t i_shape_size)
{
    if (ps_pt == NULL)
        return NULL;

    float f_x_ratio  = ((float) i_width) / (1 - (-1));
    float f_y_ratio  = (((float)i_lines) / 2) / (1 - (0));
    float f_x_offset = ((float)i_width) / (1 - (-1));
    float f_y_offset = 0;
    float f_bez_x, f_bez_y;

    float f_current_scale = 1;

    uint8_t i_last_pt = (3 * (i_pts_nbr-1) + 1);

    point_t *ps_new_pt = (point_t *)malloc( sizeof( point_t ) * i_last_pt );			// sunqueen modify
    if (ps_new_pt == NULL)
        return NULL;

    bool b_fit = true;

    /* check if the curve fit with available space */
    do {
        b_fit = true;

        for (uint8_t i_p = 0; i_p < i_last_pt; i_p++) {
            if ( i_p == 0 || i_p == 1 )
                ps_new_pt[i_p].f_x = ps_pt[i_p].f_x * f_x_ratio + f_x_offset;
            else if ( i_p == i_last_pt - 2 || i_p == i_last_pt - 1 )
                ps_new_pt[i_p].f_x = ps_pt[i_p].f_x * f_x_ratio + f_x_offset;
            else
                ps_new_pt[i_p].f_x = ps_pt[i_p].f_x * f_x_ratio * f_current_scale + f_x_offset;
            ps_new_pt[i_p].f_y = ps_pt[i_p].f_y * f_y_ratio * f_current_scale + f_y_offset;
        }

        for (float f_t = 0; f_t <= i_pts_nbr - 1; f_t += 0.1 ) {
            int8_t i_main_t = floor(f_t);
            if ( i_main_t == i_pts_nbr - 1 )
                i_main_t = i_pts_nbr - 2;
            float f_sub_t = f_t - i_main_t;

            f_bez_x = bezier_val(ps_new_pt,f_sub_t,i_main_t,x);
            f_bez_y = bezier_val(ps_new_pt,f_sub_t,i_main_t,y);

            if ( f_bez_x < ((float) i_width) / 2 ) {
                if ( abs ( f_bez_y ) > ( f_bez_x * ( 0.9 * ((float)i_lines) / ((float)i_width) ) ) )
                    b_fit = false;
            }
            else  {
                if ( abs ( f_bez_y ) > ( ( ((float)i_width) - f_bez_x ) * ( 0.9 * ((float)i_lines) / ((float)i_width) ) ) )
                    b_fit = false;
            }
        }

        if (!b_fit)
            f_current_scale = f_current_scale * 0.9;
    }
    while ((!b_fit) && (f_current_scale>0.1));

    if (!b_fit) {
        free(ps_new_pt);
        return NULL;
    }

    /* global scale shall be applied: */
    f_current_scale = f_current_scale * (0.5 + 0.5* (float)i_shape_size / 100);
    for (uint8_t i_p = 0; i_p < i_last_pt; i_p++) {
        if ( i_p == 0 || i_p == 1 )
            ps_new_pt[i_p].f_x = ps_pt[i_p].f_x * f_x_ratio + f_x_offset;
        else if ( i_p == i_last_pt - 2 || i_p == i_last_pt - 1 )
            ps_new_pt[i_p].f_x = ps_pt[i_p].f_x * f_x_ratio + f_x_offset;
        else
            ps_new_pt[i_p].f_x = ps_pt[i_p].f_x * f_x_ratio * f_current_scale + f_x_offset;
        ps_new_pt[i_p].f_y = ps_pt[i_p].f_y * f_y_ratio * f_current_scale + f_y_offset;
    }

    return ps_new_pt;
}

/*****************************************************************************
 * compute horizontal data to create scaled vertical data
 *****************************************************************************/
point_t *puzzle_H_2_scale_curve_V(int32_t i_width, int32_t i_lines, uint8_t i_pts_nbr, point_t *ps_pt, int32_t i_shape_size)
{
    if (ps_pt == NULL)
        return NULL;

    point_t *ps_bezier_scale_H = puzzle_scale_curve_H(i_lines, i_width, i_pts_nbr, ps_pt, i_shape_size);
    point_t *ps_pts_V = puzzle_curve_H_2_V(i_pts_nbr, ps_bezier_scale_H);
    free(ps_bezier_scale_H);

    return ps_pts_V;
}

/*****************************************************************************
 * compute horizontal data to create vertical data
 *****************************************************************************/
point_t *puzzle_curve_H_2_V(uint8_t i_pts_nbr, point_t *ps_pt)
{
    if (ps_pt == NULL)
        return NULL;

    point_t *ps_new_pt = (point_t *)malloc( sizeof( point_t ) * (3 * (i_pts_nbr-1) + 1) );			// sunqueen modify

    if (ps_new_pt == NULL)
        return NULL;

    for (uint8_t i=0; i < (3 * (i_pts_nbr-1) + 1); i++) {
        ps_new_pt[i].f_x = ps_pt[i].f_y;
        ps_new_pt[i].f_y = ps_pt[i].f_x;
    }

    return ps_new_pt;
}

/*****************************************************************************
 * compute horizontal data to create negative ones
 *****************************************************************************/
point_t *puzzle_curve_H_2_negative(uint8_t i_pts_nbr, point_t *ps_pt)
{
    if (ps_pt == NULL)
        return NULL;

    point_t *ps_new_pt = (point_t *)malloc( sizeof( point_t ) * (3 * (i_pts_nbr-1) + 1) );			// sunqueen modify
    if (ps_new_pt == NULL)
        return NULL;

    for (uint8_t i=0; i < (3 * (i_pts_nbr-1) + 1); i++) {
        ps_new_pt[i].f_x = ps_pt[i].f_x;
        ps_new_pt[i].f_y = -ps_pt[i].f_y;
    }

    return ps_new_pt;
}

/*****************************************************************************
 * compute vertical data to create negative ones
 *****************************************************************************/
point_t *puzzle_curve_V_2_negative(uint8_t i_pts_nbr, point_t *ps_pt)
{
    if (ps_pt == NULL)
        return NULL;

    point_t *ps_new_pt = (point_t *)malloc( sizeof( point_t ) * (3 * (i_pts_nbr-1) + 1) );			// sunqueen modify
    if (ps_new_pt == NULL)
        return NULL;

    for (uint8_t i=0; i < (3 * (i_pts_nbr-1) + 1); i++) {
        ps_new_pt[i].f_x = -ps_pt[i].f_x;
        ps_new_pt[i].f_y = ps_pt[i].f_y;
    }

    return ps_new_pt;
}

/*****************************************************************************
 * generate random bezier data
 *****************************************************************************/
point_t *puzzle_rand_bezier(uint8_t i_pts_nbr)
{
#define NB_PRIM 13
/*****************************************************************************
 * The table below contains bezier points that creates a typical
 * jigsaw puzzle piece shape. One set has been defined manually.
 * Then random modifications have been applied.
 * The best results have been selected and are included here.
 *****************************************************************************/
    const point_t ps_pt[NB_PRIM][19] = { {{ -1,    0}, { -0.708333333333333,    0},
                               {  -0.375,    -0.333333333333333}, { -0.166666666666667,    0.0833333333333333}, { -0.0833333333333334,    0.208333333333333},
                               { -0.375,    0.416666666666667}, { -0.4,    0.583333333333333}, { -0.416666666666667,    0.833333333333333},
                               { -0.25,    1}, { 0,    1}, { 0.25,    1},
                               { 0.416666666666667,    0.833333333333333}, { 0.4,    0.583333333333333}, { 0.375,    0.416666666666667},
                               { 0.0833333333333334,    0.208333333333333}, { 0.166666666666667,    0.0833333333333333}, { 0.375,    -0.333333333333333},
                               {  0.708333333333333,    0}, { 1,    0}},

                               {{ -1,    0}, { -0.708231074018077,    0.00464090724581488},
                               {  -0.323236452068492,    -0.372786060362316}, { -0.116455168200171,    0.044302770499351}, { -0.0335691043909019,    0.211488362938889},
                               { -0.437927544463254,    0.38719460194857}, { -0.465325829944998,    0.551293871552922}, { -0.483454564038933,    0.65987409733561},
                               { -0.190232837055291,    0.93567381392124}, { 0.0642797691187335,    0.936855546259066}, {0.313367868665637,   0.938012091966671},
                               {0.487146524283283,   0.816195130161918}, {0.469545566313243,   0.564387421486367}, {0.446892478470506,   0.24030153060388},
                               {0.207135456718658,   0.246041323358689}, {0.287851875888374,   0.122157561245575}, {0.492785457693622,   -0.19237501290106},
                               { 0.707786879710212,   0.000871347032899226}, {1,   0}},

                               {{ -1,    0}, { -0.704537606957651,    0.00470344095405053},
                               {  -0.435930692234854,    -0.352359270526667}, { -0.228149843936683,    0.0679948519756222}, { -0.146863413857337,    0.232442568245956},
                               { -0.400774053301818,    0.353459760810654}, { -0.422294600163745,    0.522585095895632}, { -0.436816721748459,    0.636711316521778},
                               { -0.139151386987432,    1.08020929564109}, { 0.110882572064929,    1.08261729027387}, {0.36153850539327,   1.08503127493587},
                               {0.34588115345217,   0.865990116291394}, {0.329903557511847,   0.612892723601664}, {0.308148644342904,   0.26827818823501},
                               {0.127493141873546,   0.13002329074962}, {0.214157995034913,   0.0010516930680228}, {0.419298604696494,   -0.304231373969182},
                               { 0.710915592189813,   -0.00442574861414977}, {1,   0}},

                               {{ -1,    0}, { -0.712310641244798,    -0.00176730760415818},
                               {  -0.493540738434648,    -0.309260977632173}, { -0.285884861158849,    0.102814242456153}, { -0.204387117876255,    0.264539501285563},
                               { -0.420693738052021,    0.397849004532357}, { -0.441431505287778,    0.562611714939519}, { -0.461628378308195,    0.723076990818189},
                               { -0.237390284827422,    0.937205665156549}, { 0.012635296180645,    0.941029970697368}, {0.262998571390198,   0.94485944149288},
                               {0.388416614305901,   0.85661645417048}, {0.371248440058972,   0.611257540385605}, {0.345208629600827,   0.239109662732447},
                               {0.0581354739284663,   0.176880217503811}, {0.136998743377185,   0.0517079719473858}, {0.348267592311711,   -0.283619188873049},
                               { 0.708090161530147,   0.000345266964160967}, {1,   0}},

                               {{ -1,    0}, { -0.711243094744545,    -0.00459592941542872},
                               {  -0.344045254972826,    -0.249350550360079}, { -0.133712969208732,    0.170729185550043}, { -0.0464161071620253,    0.345080177938788},
                               { -0.422103631801675,    0.334575981154338}, { -0.450380528566562,    0.498555760394576}, { -0.467099640727027,    0.595511106801977},
                               { -0.207078052226595,    0.975846125373965}, { 0.042159574981007,    0.973462055639965}, {0.287191021206139,   0.971118219914322},
                               {0.330852515542335,   0.808592956913444}, {0.310390322812144,   0.55585802623889}, {0.283433878730578,   0.222910569628582},
                               {0.164262943948071,   0.173598366742734}, {0.251741291720702,   0.049453960261478}, {0.457341230249114,   -0.24232203906962},
                               { 0.708383662881845,   0.00453591968074395}, {1,   0}},

                               {{ -1,    0}, { -0.709563566764519,    0.000504612933340335},
                               {  -0.401784990268149,    -0.401999124062464}, { -0.193592021826356,    0.0146187796927396}, { -0.111906932669809,    0.178079970851903},
                               { -0.31875772800715,    0.350308507939804}, { -0.348317101378293,    0.519642874263023}, { -0.364751907373417,    0.613791604139223},
                               { -0.261109769059908,    0.917602975781519}, { -0.0140971269841824,    0.920487199482641}, {0.239116574515885,   0.923443829366756},
                               {0.464370661288271,   0.826483978760365}, {0.447638420230199,   0.579781906213412}, {0.422345781938457,   0.206864359478527},
                               {0.125463036793575,   0.196073913812856}, {0.210079852894537,   0.0665488867084866}, {0.418467910269307,   -0.25243580242811},
                               { 0.703900021885974,   0.00330911444674605}, {1,   0}},

                               {{ -1,    0}, { -0.705550924110721,    0.00312677407583926},
                               {  -0.415041079490389,    -0.256262603613135}, { -0.206251758814373,    0.165228519752475}, { -0.127460686840124,    0.324287121648782},
                               { -0.353486555975517,    0.448219649272319}, { -0.374301593332884,    0.615673871700604}, { -0.394013085772568,    0.774250221227079},
                               { -0.28341474824943,    1.03226208905838}, { -0.0332682368974526,    1.03258310507818}, {0.21500235815775,   1.03290171371209},
                               {0.359673321091526,   0.870921326239785}, {0.339932613238046,   0.624982013252291}, {0.312186121753393,   0.279302764858672},
                               {0.115889225615101,   0.23413749518865}, {0.199563649684811,   0.112671061164123}, {0.404949947429742,   -0.185479078044395},
                               { 0.711077310890697,   -0.00496397607736578}, {1,   0}},

                               {{ -1,    0}, { -0.703393023950601,    0.00477096251262726},
                               {  -0.397655885794691,    -0.396549402674607}, { -0.188941722741602,    0.0154382717692692}, { -0.108388702754651,    0.174444497740687},
                               { -0.373390092271521,    0.482883861046198}, { -0.40085845720332,    0.649893787354158}, { -0.415216707820891,    0.73719313638733},
                               { -0.207371750103189,    0.945376369116883}, { 0.0450859051405016,    0.945770549381538}, {0.295681992651987,   0.946161823046823},
                               {0.436428045351514,   0.895032817250379}, {0.416214840162102,   0.640148265397975}, {0.392784984133714,   0.344702377534045},
                               {0.112552686103251,   0.228040049100136}, {0.197835182813393,   0.100734153702732}, {0.405083123585628,   -0.208636299638369},
                               { 0.710532321943806,   0.00118461271792703}, {1,   0}},

                               {{ -1,    0}, { -0.708545258498605,    -0.00125649641636185},
                               {  -0.337498620726249,    -0.244893936731276}, { -0.124910631945282,    0.17201407250253}, { -0.0378099534640198,    0.342827911406433},
                               { -0.388593443990334,    0.484174318751186}, { -0.418161094500799,    0.649198946145559}, { -0.437373248647437,    0.756426897124284},
                               { -0.189109020838902,    0.919750563663455}, { 0.0617320119458061,    0.92141119323056}, {0.31608889374516,   0.923095098586168},
                               {0.498311890876136,   0.848907293614162}, {0.486010157001842,   0.596632149071449}, {0.461260133020122,   0.0890763897591911},
                               {0.075676233826577,   0.15314863012444}, {0.155893607369245,   0.0261168678327565}, {0.366248653274704,   -0.307000149194794},
                               { 0.711164480468843,   0.00394203362859574}, {1,   0}},

                               {{ -1,    0}, { -0.707183721193905,    0.00108263364450203},
                               {  -0.483784802307194,    -0.278675576139177}, { -0.276928949597787,    0.142419327760986}, { -0.193170452053892,    0.312925871385917},
                               { -0.451215340488792,    0.477259970702323}, { -0.47460300367851,    0.643765731024187}, { -0.494031809431451,    0.782086864170215},
                               { -0.236790915210626,    0.959374429536679}, { 0.0132927154237516,    0.955639549881874}, {0.267150847268955,   0.951848299853113},
                               {0.394360682972295,   0.847565361471232}, {0.378470732344786,   0.601630247482969}, {0.354536849929646,   0.231195987620713},
                               {0.0517827835992971,   0.214030018332778}, {0.131796334571256,   0.0912722226051247}, {0.336621017220957,   -0.222972380306016},
                               { 0.703679022364791,   -0.00331356185794636}, {1,   0}},

                               {{ -1,    0}, { -0.71248840208346,    -0.000315316402810925},
                               {  -0.335615004340797,    -0.24819255482402}, { -0.131187942697538,    0.164054053482729}, { -0.0493962500017139,    0.3289947791894},
                               { -0.419381248020232,    0.390159881019368}, { -0.441651048160997,    0.558451191050566}, { -0.455822752006908,    0.665545758156122},
                               { -0.233491027161151,    0.962685238392525}, { 0.0133445688612305,    0.95860585518251}, {0.262151404887793,   0.954493893837471},
                               {0.353774477399895,   0.909561597589476}, {0.33709535778317,   0.660905314411181}, {0.31704981166686,   0.362061544110332},
                               {0.105412252277536,   0.191206346512902}, {0.186218651070473,   0.0649153599195794}, {0.398293919310497,   -0.266533575537957},
                               { 0.704071013216639,   0.00309631694609307}, {1,   0}},

                               {{ -1,    0}, { -0.705056079463317,    0.00448211481221729},
                               {  -0.436957920272407,    -0.370262236529651}, { -0.229712063674328,    0.0431705143488563}, { -0.148227797168837,    0.205722687925072},
                               { -0.393257971601542,    0.424195267916701}, { -0.413510880163265,    0.589027317989955}, { -0.431898814144998,    0.738680926783159},
                               { -0.283603628196569,    0.915912313032585}, { -0.0357952161759055,    0.912250885919817}, {0.219122757016883,   0.908484413381742},
                               {0.443769349276008,   0.835076661704473}, {0.426106787792343,   0.583529647320461}, {0.400589787646949,   0.220121134430258},
                               {0.160593774137044,   0.15625323679053}, {0.245514983733696,   0.0314675787386357}, {0.452432214072397,   -0.272582526914925},
                               { 0.707859045957901,   -0.00364987569003833}, {1,   0}},

                               { { -1,    0}, { -0.707920686483174,    0.00318900911649754},
                               {  -0.434372174464315,    -0.307446433658587}, { -0.22207624254243,    0.109425261995917}, { -0.137453117820789,    0.275594180895755},
                               { -0.340508174095858,    0.364631782467402}, { -0.369080584284576,    0.527714098008385}, { -0.383671882476694,    0.610996631060469},
                               { -0.233753076988816,    0.939761357928644}, { 0.0204626018463874,    0.936196353095824}, {0.268226367774715,   0.932721826949446},
                               {0.500766516589953,   0.908734435247741}, {0.488915515020803,   0.659928459184412}, {0.468202989215343,   0.225079120105809},
                               {0.106186153956061,   0.298643666003939}, {0.184680334657865,   0.170455849778656}, {0.39345790442032,   -0.1704960590812},
                               { 0.713223372514099,   -0.000707944210808817}, {1,   0}}
                               };

    if (i_pts_nbr != 7)
        return NULL;

    /* random shape */
    uint8_t i_last_pt = (3 * (i_pts_nbr-1) + 1);
    uint8_t i_item = ((uint16_t) vlc_mrand48()) %  NB_PRIM;

    point_t *ps_new_pt = (point_t *)malloc( sizeof( point_t ) * i_last_pt );			// sunqueen modify
    if (ps_new_pt == NULL)
        return NULL;

    if ((vlc_mrand48() & 1) == 1)
        for (uint8_t i=0; i < i_last_pt; i++) {
            ps_new_pt[i].f_x = ps_pt[i_item][i].f_x;
            ps_new_pt[i].f_y = ps_pt[i_item][i].f_y;
        }
    else
        for (uint8_t i=0; i < i_last_pt; i++) {
            ps_new_pt[i].f_x = ps_pt[i_item][i_last_pt-1-i].f_x;
            ps_new_pt[i].f_y = ps_pt[i_item][i_last_pt-1-i].f_y;
        }

    /* random shape size */
    float f_current_scale = 0.7 + ( (float) (vlc_mrand48() %  1001 ) / 1000 )*0.3;
    for (uint8_t i_p = 0; i_p < i_last_pt; i_p++) {
        if ( i_p != 0 && i_p != 1 && i_p != i_last_pt - 2 && i_p != i_last_pt - 1 )
            ps_new_pt[i_p].f_x = ps_new_pt[i_p].f_x * f_current_scale;
        ps_new_pt[i_p].f_y     = ps_new_pt[i_p].f_y * f_current_scale;
    }

    /* random shape shift */
    float f_offset = ( ( (float) (vlc_mrand48() %  1001 ) / 1000 ) - 0.5 ) * 0.2;
    for (uint8_t i=1; i < i_pts_nbr - 1; i++) {
        ps_new_pt[i*3-1].f_x += f_offset;
        ps_new_pt[i*3].f_x   += f_offset;
        ps_new_pt[i*3+1].f_x += f_offset;
    }

    return ps_new_pt;
}

