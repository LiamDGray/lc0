/*
  This file is part of Leela Chess Zero.
  Copyright (C) 2021 The LCZero Authors

  Leela Chess is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Leela Chess is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Leela Chess.  If not, see <http://www.gnu.org/licenses/>.

  Additional permission under GNU GPL version 3 section 7

  If you modify this Program, or any covered work, by linking or
  combining it with NVIDIA Corporation's libraries from the NVIDIA CUDA
  Toolkit and the NVIDIA CUDA Deep Neural Network library (or a
  modified version of those libraries), containing parts covered by the
  terms of the respective license agreement, the licensors of this
  Program grant you additional permission to convey the resulting work.
*/

// This is the trivial backend, which.
// Uses idea from
// https://www.chessprogramming.org/Simplified_Evaluation_Function
// for Q (but coefficients are "trained" from 1000 arbitrary test60 games).
// Returns the same P vector always ("trained" from 1 hour of test60 games).

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <memory>

#include "neural/factory.h"
#include "utils/bititer.h"
#include "utils/logging.h"

namespace lczero {
namespace {

constexpr std::array<float, 1858> kLogPolicy = {
    -3.27805f, -2.55371f, -2.46718f, -2.59232f, -2.74631f, -2.59647f, -2.47084f,
    -3.65601f, -2.09820f, -1.43034f, -3.51708f, -1.26485f, -2.36647f, -2.94045f,
    -2.40305f, -2.70842f, -2.52492f, -2.57050f, -2.48690f, -2.21723f, -2.35995f,
    -1.97193f, -1.93535f, -2.93369f, -2.62881f, -2.61207f, -2.72703f, -2.71673f,
    -2.52759f, -2.49393f, -2.22701f, -2.63556f, -2.12130f, -1.82262f, -3.50585f,
    -2.87152f, -1.57311f, -2.45245f, -2.88140f, -2.58226f, -2.62983f, -2.85953f,
    -2.63080f, -3.08031f, -2.13966f, -2.85110f, -2.12827f, -3.24672f, -2.85159f,
    -2.67270f, -2.98899f, -3.06657f, -2.71692f, -2.68217f, -2.30527f, -1.55012f,
    -2.70463f, -2.65240f, -1.78385f, -2.58179f, -1.94060f, -2.98342f, -1.57773f,
    -2.49979f, -2.94035f, -2.23586f, -2.80895f, -2.83289f, -2.76390f, -4.54929f,
    -2.25179f, -2.14827f, -3.16414f, -3.54998f, -3.06761f, -3.11185f, -3.34247f,
    -3.05679f, -2.83999f, -2.23640f, -2.66808f, -3.11225f, -2.74733f, -1.83896f,
    -2.95630f, -1.93498f, -3.40767f, -1.60610f, -2.95001f, -3.24019f, -2.87622f,
    -3.88308f, -3.14287f, -3.82922f, -2.98526f, -2.76060f, -2.11013f, -2.55618f,
    -3.16483f, -2.89776f, -3.00680f, -3.40576f, -2.92824f, -1.08351f, -2.18712f,
    -3.40693f, -3.31301f, -2.08158f, -2.03667f, -2.36173f, -1.76878f, -3.00632f,
    -2.05124f, -2.29763f, -2.66771f, -3.03798f, -2.47170f, -2.67618f, -2.86332f,
    -3.16487f, -2.63969f, -2.14089f, -2.70953f, -2.94945f, -2.52531f, -2.31311f,
    -2.39041f, -2.38423f, -2.79781f, -2.21194f, -1.83517f, -2.31660f, -1.19398f,
    -2.70912f, -2.19063f, -1.67444f, -2.92405f, -1.89225f, -3.19352f, -2.53407f,
    -2.78346f, -2.33611f, -2.86697f, -4.23423f, -2.88961f, -2.62569f, -1.95808f,
    -2.29278f, -2.36494f, -2.33724f, -2.40053f, -2.56087f, -2.83420f, -3.53338f,
    -2.18170f, -2.13917f, -2.04813f, -2.66216f, -2.12418f, -0.69842f, -2.65481f,
    -3.79830f, -2.08032f, -2.71945f, -2.10865f, -2.59930f, -2.09607f, -2.59304f,
    -1.96199f, -2.21306f, -2.00576f, -2.20327f, -2.30800f, -2.18593f, -2.14309f,
    -2.43553f, -3.97334f, -3.46230f, -1.39132f, -1.53451f, -2.93402f, -2.16152f,
    -1.20217f, -3.35988f, -2.23190f, -2.86953f, -2.25928f, -2.66984f, -2.10801f,
    -2.55898f, -1.92820f, -2.37356f, -1.55857f, -2.03181f, -2.28908f, -2.17008f,
    -1.79786f, -2.11051f, -2.25579f, -2.25968f, -2.21662f, -1.93364f, -1.75094f,
    -1.66550f, -2.72936f, -1.64063f, -1.61075f, -2.86833f, -1.78174f, -2.57420f,
    -2.09685f, -2.58283f, -1.92704f, -2.48349f, -1.69953f, -2.59193f, -1.76127f,
    -2.01200f, -2.94340f, -2.38107f, -3.08673f, -1.99493f, -2.33547f, -2.10375f,
    -2.35984f, -2.29878f, -1.96549f, -1.78870f, -1.65615f, -2.77078f, -2.80018f,
    -2.27486f, -1.53731f, -2.04792f, -3.32063f, -1.71385f, -2.84901f, -2.34161f,
    -2.99670f, -2.23259f, -3.02214f, -1.93832f, -2.77222f, -1.81773f, -2.68912f,
    -3.35246f, -2.98197f, -2.77642f, -2.82059f, -2.61695f, -2.79896f, -2.40170f,
    -2.51546f, -2.62934f, -2.31413f, -2.02635f, -1.92560f, -2.99387f, -2.57964f,
    -2.24623f, -2.52376f, -1.91827f, -3.34948f, -2.56071f, -1.65398f, -2.16174f,
    -3.18252f, -2.82216f, -3.51838f, -2.64710f, -3.88911f, -2.21975f, -3.90789f,
    -2.14583f, -3.95923f, -2.95078f, -2.62058f, -2.57345f, -3.03102f, -2.59558f,
    -2.61131f, -2.31295f, -2.28283f, -2.43661f, -2.33765f, -2.13112f, -2.71698f,
    -2.17856f, -2.11073f, -2.33474f, -2.28725f, -3.00949f, -2.64762f, -1.04994f,
    -2.69146f, -2.90213f, -3.23718f, -2.76631f, -3.25924f, -2.51251f, -3.72328f,
    -2.33565f, -2.08714f, -3.21947f, -2.85503f, -2.63804f, -2.84485f, -3.52504f,
    -2.55603f, -2.60966f, -2.76627f, -2.33495f, -2.05168f, -2.27313f, -2.22733f,
    -2.38077f, -2.58201f, -1.68292f, -2.27004f, -2.34945f, -3.15454f, -2.29580f,
    -2.00001f, -2.28900f, -3.42943f, -3.26178f, -2.78328f, -3.65967f, -3.76857f,
    -2.82601f, -2.31787f, -2.18220f, -2.66768f, -2.23966f, -2.30392f, -2.29266f,
    -3.16170f, -2.21344f, -2.27064f, -2.44117f, -2.36266f, -1.94422f, -1.93731f,
    -2.12489f, -2.01889f, -1.51826f, -2.97232f, -1.76259f, -2.73162f, -2.50737f,
    -2.17278f, -3.13102f, -2.52260f, -2.77223f, -2.61413f, -2.56946f, -2.58251f,
    -2.45132f, -2.52211f, -2.31542f, -1.88442f, -2.20100f, -2.68182f, -2.46038f,
    -3.37642f, -1.84869f, -1.97638f, -2.04854f, -2.16895f, -2.11686f, -1.75997f,
    -2.36928f, -1.71232f, -2.30947f, -2.62440f, -2.89605f, -3.20371f, -1.87585f,
    -3.72998f, -2.35252f, -3.22560f, -2.32038f, -2.96892f, -2.29096f, -2.57950f,
    -2.06247f, -2.79851f, -1.75405f, -1.94448f, -2.01396f, -2.33409f, -1.49118f,
    -1.63272f, -1.72012f, -1.75873f, -1.82517f, -1.88277f, -1.51172f, -1.95535f,
    -1.63343f, -2.66199f, -2.63170f, -2.11238f, -3.26804f, -2.48582f, -1.96174f,
    -2.38943f, -1.84035f, -2.43613f, -1.79448f, -2.44656f, -1.70873f, -2.23079f,
    -3.07955f, -2.69291f, -2.12766f, -2.08189f, -2.07082f, -2.11996f, -2.29495f,
    -2.34251f, -2.08395f, -1.99731f, -1.86997f, -1.72110f, -2.69204f, -1.74916f,
    -2.03069f, -2.18267f, -2.32030f, -2.60985f, -2.09827f, -2.55477f, -1.83743f,
    -2.57651f, -1.75630f, -2.52175f, -3.80455f, -2.50695f, -3.12213f, -2.78579f,
    -2.55680f, -2.16556f, -2.33857f, -2.61161f, -2.53169f, -2.26087f, -2.60362f,
    -2.39008f, -2.21694f, -2.07617f, -1.99662f, -2.42189f, -2.87815f, -2.15145f,
    -2.29009f, -3.10180f, -2.87811f, -2.45276f, -3.09524f, -2.79113f, -3.00962f,
    -2.39040f, -3.07481f, -1.97233f, -2.56667f, -2.98239f, -4.48054f, -2.51603f,
    -3.68408f, -2.80978f, -4.02947f, -2.30009f, -2.18421f, -2.31835f, -2.93369f,
    -2.46494f, -2.11828f, -2.10616f, -2.28425f, -2.31544f, -2.12953f, -2.09662f,
    -3.37759f, -2.30327f, -2.55404f, -1.79951f, -2.90035f, -3.11283f, -3.39870f,
    -2.44633f, -2.98232f, -2.80023f, -2.48821f, -2.88594f, -2.01654f, -2.82274f,
    -2.03482f, -2.78492f, -3.26634f, -3.24195f, -2.72334f, -2.99127f, -3.19865f,
    -3.04587f, -2.51103f, -2.31969f, -2.55876f, -2.55748f, -2.58753f, -2.47201f,
    -2.15586f, -2.12523f, -2.40960f, -2.37746f, -2.45204f, -2.89473f, -2.60158f,
    -2.42614f, -2.46064f, -2.34556f, -3.24733f, -2.44070f, -2.59262f, -2.49762f,
    -3.37558f, -3.75832f, -2.48748f, -3.80084f, -2.19600f, -3.79062f, -2.04098f,
    -3.40081f, -3.18389f, -2.68514f, -3.22397f, -2.92402f, -2.90829f, -2.52897f,
    -2.13440f, -2.08583f, -2.84989f, -2.35185f, -2.54278f, -2.46315f, -2.01244f,
    -1.96864f, -2.41030f, -2.47213f, -2.47850f, -1.83279f, -2.75409f, -2.39579f,
    -2.91434f, -2.95888f, -2.45982f, -2.54589f, -2.72006f, -3.42412f, -3.10157f,
    -2.77292f, -3.85904f, -3.18310f, -2.29223f, -2.19258f, -2.87856f, -3.52170f,
    -2.38993f, -4.64160f, -3.25123f, -3.15322f, -2.15700f, -1.91129f, -2.05896f,
    -3.87007f, -2.31107f, -2.38624f, -2.48424f, -2.48776f, -1.92920f, -2.00539f,
    -2.58454f, -2.72011f, -2.08597f, -2.33988f, -2.38105f, -3.78748f, -2.94846f,
    -2.81264f, -3.03018f, -3.92556f, -3.45293f, -2.77133f, -3.05216f, -2.55676f,
    -2.84859f, -2.82938f, -2.03113f, -2.42253f, -2.85646f, -2.17388f, -3.81336f,
    -2.56566f, -1.72219f, -1.79065f, -2.17955f, -2.09137f, -2.20016f, -2.20045f,
    -2.41179f, -2.33142f, -1.68436f, -2.00165f, -2.40213f, -1.78619f, -2.97991f,
    -2.03122f, -2.81125f, -2.70508f, -2.75108f, -2.93190f, -2.82492f, -2.93223f,
    -2.89579f, -2.66369f, -3.09179f, -1.96086f, -2.33415f, -2.70489f, -2.09119f,
    -1.74156f, -1.50930f, -1.71216f, -1.62041f, -1.75809f, -1.80507f, -2.00153f,
    -2.09758f, -2.10626f, -1.47714f, -1.75447f, -1.44928f, -2.73195f, -2.70826f,
    -2.44281f, -2.19051f, -2.65505f, -2.11028f, -2.54282f, -1.93472f, -2.45693f,
    -1.74596f, -2.18828f, -2.47454f, -2.35589f, -2.49861f, -2.31872f, -2.37263f,
    -2.17848f, -2.16253f, -2.49976f, -2.37605f, -2.41639f, -2.31335f, -2.25894f,
    -2.49608f, -2.27300f, -2.19441f, -1.77622f, -2.08721f, -2.51806f, -2.67657f,
    -2.87500f, -2.30523f, -2.55959f, -1.86961f, -2.64929f, -2.16811f, -2.43415f,
    -2.77701f, -2.21105f, -2.18239f, -2.38285f, -2.10211f, -2.01523f, -1.90136f,
    -1.96068f, -1.95664f, -1.99073f, -2.10522f, -2.22135f, -2.06296f, -2.18416f,
    -2.01866f, -1.93989f, -2.16718f, -1.68751f, -2.05182f, -2.70005f, -2.14289f,
    -2.19256f, -2.50235f, -1.96130f, -2.59285f, -1.86936f, -2.70564f, -2.43371f,
    -2.98850f, -2.71071f, -3.27384f, -2.33446f, -2.88198f, -2.54575f, -3.56300f,
    -2.35963f, -2.14711f, -2.40189f, -2.36698f, -2.25897f, -2.24260f, -2.02243f,
    -2.33412f, -2.38856f, -2.52751f, -2.29562f, -3.00430f, -2.42419f, -2.58575f,
    -1.73016f, -2.44198f, -3.69007f, -3.07589f, -2.42086f, -2.78920f, -3.15726f,
    -2.19145f, -3.34312f, -2.14677f, -2.80956f, -2.92510f, -2.54229f, -2.64030f,
    -2.71488f, -3.23822f, -2.48303f, -2.98548f, -2.42741f, -2.76244f, -2.19121f,
    -2.15651f, -2.01607f, -2.65838f, -2.36130f, -2.53446f, -2.14098f, -2.16233f,
    -2.40205f, -2.78516f, -2.52937f, -2.88440f, -1.83036f, -2.54164f, -1.68723f,
    -3.03116f, -2.99693f, -2.60829f, -2.41607f, -3.37414f, -2.98597f, -3.04663f,
    -2.39045f, -2.91719f, -2.14359f, -2.83079f, -2.96203f, -2.45407f, -2.96953f,
    -2.58018f, -2.92212f, -2.37740f, -2.69961f, -2.49857f, -2.69541f, -2.05724f,
    -2.00530f, -1.99569f, -2.69320f, -2.15570f, -2.45105f, -2.28308f, -1.95776f,
    -2.01425f, -2.57625f, -2.58345f, -2.62041f, -1.66176f, -2.22696f, -2.04774f,
    -2.95035f, -2.73967f, -2.66807f, -2.64775f, -2.73152f, -3.26277f, -2.57792f,
    -2.34761f, -3.10747f, -2.71430f, -2.19213f, -3.38385f, -2.18705f, -2.92125f,
    -2.70237f, -2.15332f, -2.88140f, -3.03359f, -2.45556f, -2.30681f, -1.91017f,
    -2.36607f, -3.24726f, -2.05841f, -2.25753f, -2.29119f, -2.23472f, -1.88084f,
    -2.05394f, -2.36612f, -2.49803f, -2.19139f, -2.28553f, -2.56190f, -2.83538f,
    -3.06285f, -2.97343f, -2.69375f, -3.27634f, -3.95293f, -3.38088f, -2.66496f,
    -3.65575f, -2.03979f, -2.48842f, -1.95172f, -2.38707f, -2.00054f, -2.07675f,
    -2.65607f, -1.79267f, -1.73012f, -1.78330f, -1.99767f, -1.89127f, -2.07869f,
    -2.11741f, -2.21555f, -2.20841f, -1.72231f, -1.86764f, -2.05040f, -1.61327f,
    -2.07715f, -1.72813f, -2.77681f, -2.37597f, -2.60204f, -2.52153f, -2.52887f,
    -2.56209f, -2.45590f, -2.00872f, -2.15372f, -2.04436f, -2.10929f, -2.16001f,
    -2.18408f, -1.82629f, -1.59137f, -1.79041f, -1.74602f, -1.95427f, -1.93570f,
    -2.06620f, -2.30465f, -2.22643f, -1.74341f, -2.34048f, -1.47648f, -2.00384f,
    -2.61990f, -2.79520f, -2.33649f, -2.32341f, -2.28627f, -2.44100f, -1.89457f,
    -2.04758f, -2.09793f, -2.17350f, -2.03410f, -2.20825f, -2.02451f, -2.03058f,
    -1.95847f, -1.89330f, -1.97147f, -2.03824f, -2.17988f, -2.34249f, -2.15968f,
    -2.23169f, -2.19646f, -2.08405f, -1.77245f, -1.60113f, -2.11544f, -1.84474f,
    -2.43358f, -2.35725f, -1.90083f, -2.44999f, -2.10366f, -2.83689f, -2.19778f,
    -2.35656f, -3.09341f, -2.25288f, -2.18496f, -2.49246f, -2.48306f, -2.10760f,
    -2.43646f, -2.28027f, -2.05602f, -2.09228f, -2.31467f, -2.35102f, -2.36965f,
    -2.28385f, -2.23430f, -2.80955f, -2.01406f, -2.05944f, -2.66423f, -3.36453f,
    -1.96616f, -2.97724f, -2.56376f, -1.98829f, -2.87213f, -2.25546f, -2.23133f,
    -2.34622f, -2.06627f, -2.45802f, -2.81215f, -2.35850f, -2.36959f, -2.10324f,
    -3.05414f, -2.08282f, -2.12720f, -1.84930f, -2.49966f, -2.12045f, -2.09591f,
    -2.12239f, -2.34968f, -2.50831f, -2.44692f, -2.36269f, -2.98886f, -1.93362f,
    -2.09213f, -1.91741f, -2.79879f, -2.67868f, -2.87130f, -2.05039f, -2.80665f,
    -2.62980f, -2.20870f, -2.70801f, -2.29572f, -2.79538f, -2.59612f, -2.37224f,
    -2.40995f, -2.43122f, -2.66385f, -2.36472f, -2.38147f, -2.33425f, -3.11712f,
    -2.10967f, -2.03662f, -2.03923f, -2.63280f, -2.25739f, -2.22740f, -2.07232f,
    -2.02925f, -2.49691f, -2.61640f, -2.45189f, -3.06772f, -2.01314f, -2.09911f,
    -2.10208f, -2.60957f, -2.65398f, -3.01437f, -2.23397f, -2.79582f, -2.93628f,
    -2.76973f, -2.25740f, -2.77257f, -2.69414f, -2.22957f, -2.51010f, -2.35899f,
    -2.71161f, -2.36220f, -2.59277f, -2.30781f, -2.76481f, -2.40330f, -2.95625f,
    -1.98644f, -2.05878f, -2.09858f, -3.31837f, -2.09829f, -2.32696f, -2.26766f,
    -1.99927f, -2.11274f, -2.48006f, -2.56481f, -2.72063f, -2.04023f, -2.19388f,
    -1.94067f, -3.56758f, -2.81211f, -2.85422f, -2.23250f, -3.82901f, -2.88714f,
    -2.95403f, -2.30589f, -2.81081f, -2.50157f, -2.01331f, -2.31846f, -2.08757f,
    -2.24877f, -2.20436f, -2.19524f, -2.44630f, -2.53404f, -2.41221f, -1.78045f,
    -1.86100f, -1.95456f, -2.91479f, -1.90417f, -2.04239f, -2.17665f, -2.14768f,
    -1.80827f, -1.87976f, -2.09031f, -2.42031f, -1.81085f, -1.79782f, -1.86047f,
    -2.76310f, -2.57748f, -2.66853f, -2.33193f, -3.13747f, -2.80059f, -2.46372f,
    -2.06273f, -3.13628f, -1.86735f, -2.68757f, -2.06358f, -2.42925f, -2.06488f,
    -2.17551f, -2.91779f, -2.19911f, -2.16090f, -1.85600f, -2.09475f, -1.81766f,
    -1.90589f, -2.00586f, -2.22196f, -2.14874f, -1.72591f, -1.71773f, -2.65898f,
    -1.85318f, -1.77443f, -2.45163f, -2.54152f, -3.17261f, -2.24711f, -3.59329f,
    -2.71727f, -2.04273f, -2.44538f, -1.96606f, -2.19677f, -2.10418f, -2.19514f,
    -1.91936f, -2.34775f, -1.77923f, -1.95359f, -1.94258f, -1.77794f, -1.87704f,
    -1.99098f, -2.17642f, -2.29225f, -2.37089f, -1.87244f, -2.12701f, -1.75107f,
    -1.67364f, -2.81174f, -2.69765f, -2.23031f, -2.44985f, -2.00851f, -2.11672f,
    -2.03517f, -2.14776f, -1.96397f, -2.28422f, -1.96835f, -2.19551f, -1.42431f,
    -2.16588f, -1.91686f, -1.82988f, -1.40790f, -2.10013f, -2.27586f, -2.36663f,
    -2.47849f, -2.38244f, -2.31331f, -1.97177f, -1.54729f, -1.92420f, -1.74783f,
    -2.03165f, -2.27844f, -2.52088f, -2.14359f, -2.03773f, -2.19033f, -1.97465f,
    -2.29625f, -1.98357f, -2.30993f, -2.22969f, -1.83153f, -2.06495f, -1.95874f,
    -1.94909f, -1.85345f, -2.04891f, -1.95348f, -2.03582f, -2.27380f, -2.58700f,
    -2.39653f, -2.33205f, -2.09450f, -2.16998f, -1.61820f, -1.95539f, -2.26379f,
    -3.03213f, -2.07728f, -2.51776f, -2.47519f, -2.29791f, -2.34987f, -2.24506f,
    -2.10877f, -2.35250f, -2.04200f, -2.34712f, -2.54766f, -2.33497f, -2.03394f,
    -2.21075f, -2.75461f, -1.96144f, -2.07166f, -1.89114f, -2.23968f, -2.04990f,
    -2.05186f, -2.11099f, -2.53494f, -2.53646f, -2.40433f, -2.15578f, -2.96947f,
    -2.03372f, -1.74629f, -2.09146f, -2.44340f, -2.63727f, -3.22198f, -2.24365f,
    -2.80178f, -2.54007f, -2.27926f, -2.29653f, -2.42813f, -2.36442f, -2.41354f,
    -2.23481f, -2.37359f, -2.35228f, -2.30055f, -2.27875f, -2.29771f, -2.71068f,
    -1.91342f, -2.06465f, -1.89668f, -2.51739f, -2.12320f, -2.14759f, -2.01413f,
    -2.15683f, -2.53451f, -2.55102f, -2.19632f, -2.72338f, -2.03579f, -1.81866f,
    -2.08212f, -2.96693f, -2.76705f, -2.86854f, -2.27029f, -2.85617f, -2.81300f,
    -2.09222f, -2.28959f, -2.10792f, -2.19547f, -2.19424f, -2.24168f, -2.13982f,
    -2.07873f, -2.19986f, -2.01142f, -2.34383f, -2.17709f, -1.74786f, -1.86743f,
    -1.78310f, -2.36384f, -1.96474f, -2.15577f, -2.20774f, -1.83719f, -1.91188f,
    -2.25855f, -2.15191f, -2.42236f, -1.97486f, -1.68240f, -1.84782f, -2.60395f,
    -2.46040f, -2.64373f, -2.16081f, -2.56212f, -2.65208f, -2.39016f, -1.96899f,
    -2.37672f, -1.98351f, -2.11495f, -2.16868f, -2.13437f, -1.98334f, -2.19722f,
    -2.22096f, -2.40551f, -2.06782f, -1.76509f, -1.87992f, -1.82811f, -2.27810f,
    -1.97049f, -2.12135f, -2.23576f, -2.21375f, -1.87601f, -1.84173f, -1.99004f,
    -2.26579f, -1.92204f, -1.53696f, -1.94886f, -2.44900f, -2.42089f, -2.35997f,
    -2.14107f, -2.86418f, -2.62388f, -2.29501f, -1.82484f, -2.15865f, -1.95074f,
    -2.07764f, -2.09247f, -2.14312f, -1.78663f, -2.28377f, -2.24162f, -1.87493f,
    -1.73979f, -1.76333f, -1.81149f, -1.85047f, -1.96739f, -2.04565f, -2.17483f,
    -2.11464f, -1.74922f, -1.71497f, -2.12079f, -1.84692f, -1.47210f, -2.05774f,
    -2.39308f, -2.39104f, -2.02159f, -2.80460f, -2.61564f, -1.90047f, -2.33373f,
    -2.00211f, -2.09685f, -2.16511f, -2.26695f, -1.58998f, -2.13473f, -1.70681f,
    -1.85100f, -1.77046f, -1.68764f, -1.81053f, -1.92529f, -2.04866f, -2.17788f,
    -2.15314f, -1.87239f, -1.95897f, -2.00624f, -1.45804f, -2.81546f, -2.43162f,
    -2.00752f, -2.21081f, -1.69523f, -2.27725f, -1.66769f, -2.35110f, -1.66064f,
    -2.28703f, -1.80589f, -2.22687f, -1.33695f, -2.02242f, -1.90671f, -1.77027f,
    -1.59172f, -2.10972f, -2.31081f, -2.43459f, -2.47780f, -2.53299f, -2.40122f,
    -2.10230f, -2.04187f, -1.85619f, -1.97726f, -2.29052f, -2.21670f, -2.38972f,
    -1.89230f, -2.39340f, -1.84567f, -2.54204f, -2.07184f, -1.91261f, -2.36088f,
    -1.44757f, -2.22705f, -1.93230f, -2.04882f, -1.85666f, -1.72142f, -2.04852f,
    -2.18148f, -2.47233f, -2.57103f, -2.66228f, -2.36146f, -2.10443f, -2.17253f,
    -2.14498f, -2.13797f, -2.20457f, -2.37645f, -2.42756f, -2.22670f, -2.44680f,
    -2.06698f, -2.49345f, -2.06786f, -2.23341f, -1.95126f, -2.39432f, -1.68125f,
    -2.15135f, -2.40909f, -1.85258f, -2.04602f, -1.86844f, -2.05475f, -2.06725f,
    -2.01961f, -2.19149f, -2.48149f, -2.65855f, -2.45480f, -2.19928f, -2.50001f,
    -2.09398f, -2.13474f, -2.05691f, -2.40477f, -2.35070f, -2.38167f, -2.37739f,
    -2.11717f, -2.05558f, -2.42946f, -2.07644f, -2.05003f, -1.76712f, -2.37327f,
    -1.71331f, -2.13889f, -2.12451f, -1.72115f, -1.99994f, -1.78942f, -2.02653f,
    -2.01901f, -2.08600f, -2.01737f, -2.15539f, -2.50478f, -2.43162f, -2.14439f,
    -2.68323f, -1.82002f, -2.02668f, -1.80627f, -2.23072f, -2.21415f, -2.24463f,
    -2.15904f, -2.24000f, -2.15755f, -2.37529f, -2.03020f, -2.18104f, -1.83134f,
    -2.28310f, -1.80883f, -2.13150f, -1.90072f, -1.74498f, -1.93361f, -1.76177f,
    -2.26838f, -2.01060f, -2.07943f, -2.16495f, -1.98597f, -2.04789f, -2.32455f,
    -2.22731f, -2.35536f, -1.86481f, -1.95994f, -1.94230f, -2.89104f, -1.96322f,
    -2.08665f, -2.06367f, -2.05313f, -2.15806f, -1.96075f, -2.20531f, -2.07987f,
    -1.55964f, -2.29668f, -1.76706f, -2.08963f, -1.79572f, -1.70844f, -1.77588f,
    -1.68404f, -2.03263f, -1.90899f, -2.07456f, -2.22453f, -2.23994f, -1.87902f,
    -1.84461f, -2.06319f, -2.12513f, -1.90459f, -1.87878f, -2.01137f, -2.64095f,
    -1.95214f, -1.95016f, -1.92596f, -2.03448f, -1.81453f, -2.15834f, -1.80823f,
    -2.28257f, -1.95188f, -1.40515f, -2.21123f, -1.68100f, -1.78388f, -1.56169f,
    -1.68899f, -1.58597f, -1.82104f, -1.98177f, -2.11101f, -2.21825f, -2.16944f,
    -1.79568f, -1.70052f, -2.12545f, -1.76546f, -1.88302f, -1.88185f, -1.81223f,
    -1.97519f, -1.80843f, -2.01822f, -1.72436f, -2.16737f, -1.79040f, -2.17144f,
    -1.97642f, -1.08338f, -2.09722f, -1.55166f, -1.58894f, -1.67195f, -1.79258f,
    -1.91590f, -2.07872f, -2.17675f, -2.22306f, -2.34160f, -1.85982f, -1.92394f,
    -1.67892f, -1.81541f, -1.97390f, -1.62706f, -2.00251f, -1.44317f, -2.06995f,
    -1.47269f, -2.00043f, -1.57277f, -1.98441f, -1.65323f, -1.94968f, -1.32563f,
    -1.76913f, -1.54770f, -1.45107f, -1.55195f, -1.94615f, -2.22312f, -2.17876f,
    -2.18266f, -2.22343f, -2.20101f, -2.17880f, -1.82719f, -1.91612f, -1.71975f,
    -2.01348f, -1.67986f, -2.00790f, -1.66986f, -2.02330f, -1.73576f, -1.26400f,
    -1.92733f, -1.09162f, -1.92195f, -1.52879f, -1.53502f, -1.49707f, -1.27697f,
    -1.89492f, -1.85442f, -2.12911f, -2.16070f, -2.14918f, -2.17024f, -2.14222f,
    -1.98419f, -1.95113f, -2.03801f, -1.68961f, -2.05959f, -1.87573f, -2.10187f,
    -1.75431f, -1.99309f, -1.43739f, -2.07272f, -1.32327f, -2.06925f, -1.72851f,
    -1.46024f, -1.58201f, -1.53596f, -1.51227f, -2.09855f, -1.84421f, -1.86674f,
    -2.12931f, -2.21361f, -2.19483f, -2.15773f, -2.01178f, -2.02010f, -2.02189f,
    -2.00588f, -1.78924f, -1.92400f, -2.19067f, -1.89411f, -1.97522f, -1.36829f,
    -2.15518f, -1.42762f, -2.11017f, -1.71658f, -1.56530f, -1.66244f, -1.64796f,
    -1.65544f, -2.08416f, -2.15704f, -1.86680f, -1.85526f, -2.18876f, -2.25134f,
    -2.19230f, -1.91536f, -1.92085f, -1.93780f, -1.89535f, -1.95840f, -1.81512f,
    -2.05763f, -1.73630f, -1.93331f, -1.37147f, -2.23705f, -1.34394f, -1.88755f,
    -1.63986f, -1.61512f, -1.73315f, -1.64673f, -1.87312f, -2.06245f, -2.13445f,
    -2.11927f, -1.79248f, -1.81999f, -2.19646f, -2.14435f, -1.60343f, -1.66710f,
    -1.72212f, -1.75136f, -1.70293f, -1.82036f, -1.65613f, -1.91504f, -1.79989f,
    -1.34902f, -2.02918f, -1.33355f, -1.85791f, -1.47304f, -1.39559f, -1.51913f,
    -1.52185f, -1.86153f, -1.90207f, -1.93224f, -1.95073f, -1.90642f, -1.60040f,
    -1.60577f, -1.98155f, -1.60383f, -1.71976f, -1.72190f, -1.69195f, -1.80138f,
    -1.56773f, -1.93684f, -1.55984f, -1.98119f, -1.75877f, -1.03839f, -1.99725f,
    -1.13111f, -1.28862f, -1.39230f, -1.59500f, -1.42287f, -1.83753f, -1.92297f,
    -1.94480f, -1.87559f, -1.89891f, -1.51436f, -1.51757f, -1.61669f, -1.77426f,
    -1.58805f, -1.80870f, -1.50397f, -1.82487f, -1.47130f, -1.86784f, -1.50498f,
    -1.93723f, -1.76847f, -1.10059f, -2.00065f, -1.24889f, -1.42306f, -1.55937f,
    -1.85574f, -1.93160f, -1.96405f, -1.91510f, -1.99475f, -2.03417f, -1.63940f,
    -1.46124f, -2.51354f, -3.05328f, -0.23796f, -1.09853f, -1.69816f, -0.88060f,
    -2.59322f, -2.72630f, -1.39634f, -2.47534f, -2.93509f, -0.40550f, -1.30283f,
    -2.31982f, -0.32459f, -1.27121f, -2.06339f, -1.59923f, -2.71933f, -3.21009f,
    -0.09426f, -1.33884f, -1.94104f, -0.13521f, -1.30427f, -2.12129f, -1.50089f,
    -2.55270f, -2.85786f, -0.05611f, -0.82796f, -2.33907f, -0.21596f, -1.13357f,
    -1.38835f, -1.29421f, -2.32614f, -2.77775f, -0.70767f, -1.27214f, -2.37027f,
    -0.26273f, -0.95609f, -1.41015f, -1.10089f, -2.11966f, -2.40177f, -0.35224f,
    -1.20165f, -1.40952f, -0.0f,     -0.65666f, -1.98171f, -1.05692f, -2.01931f,
    -2.43228f, -0.0f,     -2.29416f, -3.49818f, -0.46989f, -1.00615f, -1.49424f,
    -1.23419f, -2.09375f, -2.43350f};

constexpr std::array<float, 64> kPawns = {
    -0.00000f, -0.00000f, -0.00000f, -0.00000f, 0.00000f,  -0.00000f, 0.00000f,
    -0.00000f, 0.06662f,  0.09583f,  0.06643f,  0.05536f,  0.02236f,  0.04939f,
    0.09071f,  0.09352f,  0.08847f,  0.08068f,  0.07738f,  0.05534f,  0.06063f,
    0.06393f,  0.08791f,  0.09560f,  0.07608f,  0.08692f,  0.06337f,  0.07179f,
    0.07750f,  0.07100f,  0.08159f,  0.08283f,  0.14966f,  0.09968f,  0.10335f,
    0.10362f,  0.08502f,  0.11313f,  0.08158f,  0.10561f,  0.13946f,  0.12644f,
    0.12511f,  0.11497f,  0.13403f,  0.09999f,  0.12569f,  0.14075f,  0.16155f,
    0.12969f,  0.16869f,  0.17744f,  0.17258f,  0.19959f,  0.14792f,  0.15976f,
    0.00000f,  -0.00000f, 0.00000f,  0.00000f,  -0.00000f, -0.00000f, -0.00000f,
    -0.00000f};
constexpr std::array<float, 64> kKnights = {
    0.12549f, 0.05358f, 0.06001f, 0.08798f, 0.09084f, 0.07007f, 0.05983f,
    0.07110f, 0.03532f, 0.08703f, 0.13308f, 0.07691f, 0.11283f, 0.06292f,
    0.08848f, 0.05982f, 0.07493f, 0.10743f, 0.10747f, 0.12312f, 0.11972f,
    0.10002f, 0.07905f, 0.06539f, 0.12239f, 0.14532f, 0.08843f, 0.12103f,
    0.10833f, 0.13367f, 0.09003f, 0.07247f, 0.12163f, 0.16514f, 0.15197f,
    0.11901f, 0.14494f, 0.13974f, 0.12081f, 0.10957f, 0.10746f, 0.11014f,
    0.15582f, 0.22478f, 0.15931f, 0.15341f, 0.11198f, 0.07076f, 0.16764f,
    0.14658f, 0.20785f, 0.12558f, 0.10667f, 0.19004f, 0.07353f, 0.14162f,
    0.03220f, 0.06356f, 0.13316f, 0.12845f, 0.14233f, 0.19931f, 0.07425f,
    0.20774f,
};
constexpr std::array<float, 64> kBishops = {
    0.08299f, 0.11050f, 0.11387f, 0.12347f, 0.13993f, 0.10414f, 0.18594f,
    0.06085f, 0.10235f, 0.15733f, 0.13970f, 0.13631f, 0.10189f, 0.17399f,
    0.14002f, 0.10948f, 0.14439f, 0.13286f, 0.15316f, 0.13379f, 0.13762f,
    0.13907f, 0.11989f, 0.12127f, 0.15578f, 0.13964f, 0.16643f, 0.14614f,
    0.12861f, 0.15553f, 0.16397f, 0.09271f, 0.18553f, 0.14091f, 0.18698f,
    0.15018f, 0.15590f, 0.12655f, 0.14573f, 0.10276f, 0.19904f, 0.15973f,
    0.13077f, 0.14071f, 0.15390f, 0.11180f, 0.10273f, 0.19621f, 0.14963f,
    0.14949f, 0.12911f, 0.11972f, 0.17507f, 0.14455f, 0.10058f, 0.11797f,
    0.15988f, 0.14084f, 0.15436f, 0.24262f, 0.12838f, 0.15251f, 0.10853f,
    0.14240f,
};
constexpr std::array<float, 64> kRooks = {
    0.19343f, 0.22010f, 0.19814f, 0.20439f, 0.20660f, 0.20584f, 0.19275f,
    0.20042f, 0.18159f, 0.19006f, 0.19286f, 0.19677f, 0.22751f, 0.22487f,
    0.19256f, 0.16757f, 0.19102f, 0.23716f, 0.21167f, 0.19747f, 0.23355f,
    0.21321f, 0.17478f, 0.17279f, 0.15728f, 0.16795f, 0.26422f, 0.22453f,
    0.24422f, 0.21715f, 0.19039f, 0.24305f, 0.18434f, 0.25995f, 0.25855f,
    0.24373f, 0.25450f, 0.23517f, 0.20909f, 0.22781f, 0.19181f, 0.26571f,
    0.26481f, 0.21262f, 0.25547f, 0.22559f, 0.22430f, 0.23066f, 0.28646f,
    0.25282f, 0.25758f, 0.21276f, 0.25720f, 0.26076f, 0.25661f, 0.24443f,
    0.23237f, 0.21318f, 0.23230f, 0.19967f, 0.21947f, 0.22544f, 0.23956f,
    0.21579f,
};
constexpr std::array<float, 64> kQueens = {
    0.23063f, 0.23157f, 0.25371f, 0.27579f, 0.27878f, 0.23600f, 0.29552f,
    0.29963f, 0.27729f, 0.29837f, 0.29026f, 0.25105f, 0.27772f, 0.28502f,
    0.29344f, 0.24009f, 0.29366f, 0.28859f, 0.27538f, 0.27713f, 0.26159f,
    0.28383f, 0.28749f, 0.24996f, 0.35152f, 0.26595f, 0.26428f, 0.30264f,
    0.29376f, 0.29841f, 0.27352f, 0.29242f, 0.30948f, 0.30742f, 0.30822f,
    0.31232f, 0.31701f, 0.28862f, 0.28218f, 0.29562f, 0.30423f, 0.33840f,
    0.29070f, 0.29734f, 0.25349f, 0.27276f, 0.24977f, 0.27319f, 0.27835f,
    0.35061f, 0.33633f, 0.29402f, 0.32144f, 0.33461f, 0.29777f, 0.28501f,
    0.31223f, 0.33044f, 0.33788f, 0.26788f, 0.29851f, 0.28789f, 0.31030f,
    0.28824f,
};
constexpr std::array<float, 64> kKings = {
    0.02852f,  0.00453f,  -0.05309f, -0.02416f, -0.14581f, -0.01472f, -0.02206f,
    0.02207f,  0.03712f,  0.02324f,  -0.02501f, -0.06653f, -0.07605f, -0.01135f,
    0.03666f,  -0.02999f, 0.00700f,  -0.02668f, -0.06998f, -0.02305f, -0.03816f,
    -0.00129f, -0.08264f, 0.01139f,  -0.08866f, -0.01720f, -0.03161f, -0.03092f,
    -0.01507f, 0.00172f,  -0.03457f, 0.02657f,  -0.00569f, 0.00000f,  0.00341f,
    -0.00108f, -0.01445f, -0.02948f, -0.00883f, 0.00954f,  0.01116f,  -0.01762f,
    0.01088f,  -0.00005f, -0.00275f, 0.00038f,  0.00219f,  0.01970f,  0.00000f,
    -0.00024f, -0.01798f, -0.00339f, -0.00226f, 0.00842f,  0.03543f,  -0.00122f,
    -0.00000f, 0.00532f,  -0.00000f, 0.00002f,  0.00138f,  0.00571f,  -0.00078f,
    0.00145f,
};
constexpr std::array<float, 64> kKingsEndgame = {
    -0.03908f, -0.02837f, -0.02194f, -0.03649f, -0.04754f, -0.03390f, -0.03172f,
    0.02852f,  -0.02071f, -0.01429f, -0.02296f, -0.01087f, -0.02774f, -0.01505f,
    -0.00469f, -0.03894f, -0.03979f, 0.02244f,  -0.00705f, -0.01847f, -0.00316f,
    -0.04952f, 0.01103f,  -0.00487f, 0.01769f,  0.01299f,  0.03068f,  -0.01422f,
    -0.00579f, -0.01817f, 0.01946f,  -0.01716f, 0.04860f,  0.01099f,  0.05517f,
    0.06880f,  0.00036f,  0.03165f,  0.07524f,  -0.00409f, 0.01840f,  0.04146f,
    0.04435f,  0.09172f,  0.08918f,  0.03642f,  0.04095f,  0.00081f,  0.07254f,
    0.03901f,  0.07186f,  0.06869f,  0.05215f,  0.00621f,  0.02240f,  -0.00463f,
    -0.01242f, -0.02068f, 0.01925f,  0.03103f,  0.02797f,  -0.02299f, -0.01944f,
    -0.04503f};

float DotProduct(uint64_t plane, const std::array<float, 64>& weights) {
  float result = 0.0f;
  for (auto idx : IterateBits(plane)) result += weights[idx];
  return result;
}

int NumBits(uint64_t x) {
  using Iterator = BitIterator<int>;
  return std::distance(Iterator(x), Iterator(0));
}

class TrivialNetworkComputation : public NetworkComputation {
 public:
  void AddInput(InputPlanes&& input) override {
    float q = 0.0f;
    q += DotProduct(input[0].mask, kPawns);
    q -= DotProduct(ReverseBytesInBytes(input[6].mask), kPawns);
    q += DotProduct(input[1].mask, kKnights);
    q -= DotProduct(ReverseBytesInBytes(input[7].mask), kKnights);
    q += DotProduct(input[2].mask, kBishops);
    q -= DotProduct(ReverseBytesInBytes(input[8].mask), kBishops);
    q += DotProduct(input[3].mask, kRooks);
    q -= DotProduct(ReverseBytesInBytes(input[9].mask), kRooks);
    q += DotProduct(input[4].mask, kQueens);
    q -= DotProduct(ReverseBytesInBytes(input[10].mask), kQueens);
    const bool endgame =
        (input[4].mask == 0 ||
         (input[3].mask == 0 && NumBits(input[1].mask | input[2].mask) <= 1)) &&
        (input[10].mask == 0 ||
         (input[9].mask == 0 && NumBits(input[7].mask | input[8].mask) <= 1));
    if (endgame) {
      q += DotProduct(input[5].mask, kKingsEndgame);
      q -= DotProduct(ReverseBytesInBytes(input[11].mask), kKingsEndgame);
    } else {
      q += DotProduct(input[5].mask, kKings);
      q -= DotProduct(ReverseBytesInBytes(input[11].mask), kKings);
    }
    // Multiply Q by 10, otherwise evals too low. :-/
    q_.push_back(2.0f / (1.0f + std::exp(q * -10.0f)) - 1.0f);
  }

  void ComputeBlocking() override {}

  int GetBatchSize() const override { return q_.size(); }

  float GetQVal(int sample) const override { return q_[sample]; }

  float GetDVal(int) const override { return 0.0f; }

  float GetMVal(int /* sample */) const override { return 0.0f; }

  float GetPVal(int /* sample */, int move_id) const override {
    return kLogPolicy[move_id];
  }

 private:
  std::vector<float> q_;
};

class TrivialNetwork : public Network {
 public:
  TrivialNetwork(const OptionsDict& options)
      : capabilities_{
            static_cast<pblczero::NetworkFormat::InputFormat>(
                options.GetOrDefault<int>(
                    "input_mode",
                    pblczero::NetworkFormat::INPUT_CLASSICAL_112_PLANE)),
            pblczero::NetworkFormat::OUTPUT_CLASSICAL,
            pblczero::NetworkFormat::MOVES_LEFT_NONE} {}
  std::unique_ptr<NetworkComputation> NewComputation() override {
    return std::make_unique<TrivialNetworkComputation>();
  }
  const NetworkCapabilities& GetCapabilities() const override {
    return capabilities_;
  }

 private:
  NetworkCapabilities capabilities_{
      pblczero::NetworkFormat::INPUT_CLASSICAL_112_PLANE,
      pblczero::NetworkFormat::OUTPUT_CLASSICAL,
      pblczero::NetworkFormat::MOVES_LEFT_NONE};
};
}  // namespace

std::unique_ptr<Network> MakeTrivialNetwork(
    const std::optional<WeightsFile>& /*weights*/, const OptionsDict& options) {
  return std::make_unique<TrivialNetwork>(options);
}

REGISTER_NETWORK("trivial", MakeTrivialNetwork, 4)

}  // namespace lczero
