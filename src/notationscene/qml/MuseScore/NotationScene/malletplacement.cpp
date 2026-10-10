/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#include "malletplacement.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <set>
#include <sstream>
#include <iomanip>
#include <limits>
#include <tuple>
#include <cctype>

namespace mu::notation::mallet {
static bool sharp(int pitch) { const int pc = pitch % 12; return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10; }
static double distance(Point a, Point b) { return std::hypot(a.x - b.x, a.y - b.y); }
const Bar* Keyboard::bar(int pitch) const {
    for (const auto& b : bars) if (b.pitch == pitch) return &b;
    return nullptr;
}
std::string pitchName(int pitch) {
    static const char* names[] { "C", "C♯", "D", "E♭", "E", "F", "F♯", "G", "A♭", "A", "B♭", "B" };
    return names[std::clamp(pitch, 0, 127) % 12] + std::to_string(pitch / 12 - 1);
}
Keyboard keyboard(int low, int high, bool metal) {
    Keyboard k;
    k.low = std::clamp(low, 0, 127); k.high = std::clamp(high, k.low, 127);
    // Estimated graduated geometry. Shared upper-row front edge overlaps the
    // natural row by 12%, with the rest of the accidental extending behind it.
    // Generate neighbours even for a range beginning/ending on an accidental.
    std::vector<Bar> naturals;
    double x = 0;
    for (int p = k.low - 2; p <= std::min(129, k.high + 2); ++p) {
        if (p < 0 || sharp(p)) continue;
        const double t = std::clamp((p - k.low) / double(std::max(1, k.high - k.low)), 0.0, 1.0);
        const double width = metal ? 4.0 - t * 1.0 : 7.0 - t * 3.0;
        const double length = metal ? 38.0 - t * 18.0 : 62.0 * std::pow(0.38, t);
        naturals.push_back({ p, false, x, 62, width, length });
        x += width + 0.4;
    }
    for (int p = k.low; p <= k.high; ++p) {
        if (!sharp(p)) {
            for (const auto& b : naturals) if (b.pitch == p) k.bars.push_back(b);
        } else {
            const Bar* left = nullptr; const Bar* right = nullptr;
            for (const auto& b : naturals) { if (b.pitch == p - 1) left = &b; if (b.pitch == p + 1) right = &b; }
            if (!left || !right) continue;
            const double width = (left->width + right->width) * 0.46;
            const double length = (left->length + right->length) / 2;
            const double center = (left->strike().x + right->strike().x) / 2;
            k.bars.push_back({ p, true, center - width / 2, 62 + length * 0.12 - length, width, length });
        }
    }
    double begin = 1e9, end = 0;
    for (const auto& b : k.bars) { begin = std::min(begin, b.x); end = std::max(end, b.x + b.width); k.front = std::max(k.front, b.y + b.length); }
    for (auto& b : k.bars) b.x -= begin;
    k.width = end - begin;
    return k;
}
std::string number(double value, int decimals) {
    std::ostringstream out; out << std::fixed << std::setprecision(decimals) << value; return out.str();
}
WrittenSticking parseWrittenSticking(const std::string& text, size_t notes, bool reverseNumbering) {
    WrittenSticking result;bool hands=false,numbers=false;
    if(text.empty()) {result.required.assign(notes,0);return result;}
    for(unsigned char c:text) {
        if(std::isspace(c) || c=='.' || c=='-') continue;
        if(c=='?' || c=='_') result.required.push_back(0);
        else if(c=='L' || c=='l') {result.required.push_back(-1);hands=true;}
        else if(c=='R' || c=='r') {result.required.push_back(-2);hands=true;}
        else if(c>='1' && c<='4') {result.required.push_back(reverseNumbering?5-(c-'0'):c-'0');numbers=true;}
        else {result.unknown=true;result.reason="Unsupported sticking syntax or mallet number";return result;}
    }
    if(result.required.size()!=notes || (hands && numbers)) {
        result.unknown=true;result.reason=hands && numbers?"Mixed hand/number convention is ambiguous":"Sticking needs one explicit or unknown slot per written note";
    }
    return result;
}
static void issue(Pose& p, std::string key, std::string text, int severity) {
    p.issues.push_back({std::move(key), std::move(text), severity}); p.severity = std::max(p.severity, severity);
    if (severity == 2) p.valid = false;
}
static bool better(const Pose& a, const Pose& b) {
    if (a.valid != b.valid) return a.valid;
    if (a.severity != b.severity) return a.severity < b.severity;
    if (std::abs(a.cost - b.cost) > 1e-7) return a.cost < b.cost;
    return a.mallets < b.mallets;
}
static double pointSegment(Point p, Point a, Point b) {
    const double dx=b.x-a.x, dy=b.y-a.y, dz=b.z-a.z;
    const double length=dx*dx+dy*dy+dz*dz;
    const double t=length > 1e-9 ? std::clamp(((p.x-a.x)*dx+(p.y-a.y)*dy+(p.z-a.z)*dz)/length,0.0,1.0) : 0;
    return std::sqrt(std::pow(p.x-a.x-t*dx,2)+std::pow(p.y-a.y-t*dy,2)+std::pow(p.z-a.z-t*dz,2));
}
static double shaftDistance(Point a, Point b, Point c, Point d) {
    // Bounded sampling along the finite shafts, followed by analytic distance
    // to the other segment. This is an estimated clearance, not exact motion.
    double minimum = std::numeric_limits<double>::max();
    for (int n=0;n<=12;++n) {
        const double t=n/12.0;
        minimum=std::min({minimum,pointSegment({a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t},c,d),
            pointSegment({c.x+(d.x-c.x)*t,c.y+(d.y-c.y)*t,c.z+(d.z-c.z)*t},a,b)});
    }
    return minimum;
}
static void transition(Pose& pose, const Pose* neighbor, double seconds, const Player& player, const char* direction) {
    if (!neighbor || neighbor->pitches.empty() || !neighbor->valid) return;
    double longest=0;
    for (int m=0;m<4;++m) if (pose.active[m] && neighbor->active[m])
        longest=std::max(longest,distance(pose.targets[m],neighbor->targets[m]));
    longest=std::max(longest,std::abs(pose.body.x-neighbor->body.x));
    pose.movement=std::max(pose.movement,longest);
    pose.preparation=std::max(pose.preparation,longest/std::max(1.0,player.travelSpeed));
    pose.cost += longest*.15;
    if (seconds > 0 && longest/seconds > player.travelSpeed) {
        issue(pose,std::string("transition-")+direction,std::string(direction)+" attack: "+number(longest)+" cm travel in "+number(seconds*1000,0)
            +" ms exceeds your estimated "+number(player.travelSpeed,0)+" cm/s travel setting. Allow preparation or choose a closer assignment.",1);
        pose.cost += 20*std::min(5.0,longest/seconds/player.travelSpeed-1);
    }
}
static Pose evaluate(const Keyboard& keyboard, const std::vector<int>& pitches, const std::vector<int>& ids,
                     const Player& player, const std::array<double,4>& fractions,
                     const Pose* previous, double previousSeconds, const Pose* next, double nextSeconds) {
    Pose pose; pose.pitches=pitches; pose.mallets=ids; pose.fractions=fractions;
    std::vector<double> centers;
    for (size_t i=0;i<pitches.size();++i) {
        const auto* bar=keyboard.bar(pitches[i]);
        if (!bar) { issue(pose,"range",pitchName(pitches[i])+" is outside this instrument's range.",2); continue; }
        const double margin=std::clamp((player.head*.5+.3)/bar->length,.02,.25);
        pose.fractions[i]=std::clamp(fractions[i],margin,1-margin);
        Point target=bar->strike(); target.y=bar->y+bar->length*pose.fractions[i];
        pose.targets[ids[i]-1]=target; pose.active[ids[i]-1]=true; centers.push_back(target.x);
        const double f=pose.fractions[i];
        if (std::abs(f-.224)<.055 || std::abs(f-.776)<.055) {
            issue(pose,"node","Mallet "+std::to_string(ids[i])+" on "+pitchName(pitches[i])+" is near an estimated vibration node; move the strike point for fuller tone.",1);
            pose.cost+=10;
        } else if (f < .30 || f > .70) {
            issue(pose,"edge","Mallet "+std::to_string(ids[i])+" on "+pitchName(pitches[i])+" uses an end access zone. Easier reach can trade away central-strike tone.",0);
            pose.cost+=4;
        } else pose.cost+=std::abs(f-.5)*5;
    }
    pose.body={keyboard.width/2,keyboard.front+player.bodyDistance};
    if (!centers.empty()) {
        const auto [left,right]=std::minmax_element(centers.begin(),centers.end());
        pose.body.x=std::clamp((*left+*right)/2+player.bodyOffset,12.0,std::max(12.0,keyboard.width-12));
    }
    // Follow the tapered front at the player's register, not the longest
    // bass bar's front across the entire keyboard. This keeps stance distance
    // physically meaningful while the player follows treble notes.
    const Bar* nearest=nullptr;double nearestDistance=std::numeric_limits<double>::max();
    for(const auto& bar:keyboard.bars) if(!bar.accidental) {
        const double d=std::abs(bar.strike().x-pose.body.x);
        if(d<nearestDistance) {nearestDistance=d;nearest=&bar;}
    }
    if(nearest) pose.body.y=nearest->y+nearest->length+player.bodyDistance;
    for (int hand=0;hand<2;++hand) {
        const std::string name=hand ? "Right" : "Left";
        const int a=hand*2,b=a+1; std::vector<Point> targets;
        if (pose.active[a]) targets.push_back(pose.targets[a]);
        if (pose.active[b]) targets.push_back(pose.targets[b]);
        Point midpoint{pose.body.x+(hand ? 21 : -21),keyboard.front+8};
        if (!targets.empty()) {
            midpoint={};for (const auto& t:targets) {midpoint.x+=t.x/targets.size();midpoint.y+=t.y/targets.size();}
        }
        if (targets.size()==2) {
            const double dx=pose.targets[b].x-pose.targets[a].x,dy=pose.targets[b].y-pose.targets[a].y;
            pose.openings[hand]=distance(targets[0],targets[1]);
            pose.rotations[hand]=std::atan2(dy,std::max(.001,std::abs(dx)))*180/3.141592653589793;
            if (dx < -.5) {
                issue(pose,"order",name+" hand reverses its inner/outer head order. This special technique is outside the supported neutral grip model, so feasibility is unknown.",1);
                pose.uncertain=true; pose.valid=false;
                pose.cost+=80;
            }
        }
        const double half=pose.openings[hand]/2;
        pose.wrists[hand]={midpoint.x,midpoint.y+std::sqrt(std::max(25.0,player.shaft*player.shaft-half*half)),8};
        pose.shoulders[hand]={pose.body.x+(hand ? 21 : -21),pose.body.y,12};
        pose.reaches[hand]=distance(pose.shoulders[hand],pose.wrists[hand]);
        // Separate inner thumb/index and outer ring/little holding positions.
        // Cross grips have an intentional local shaft crossing; do not classify
        // that normal hold as a collision. Fingers share these same anchors.
        const double angle=pose.rotations[hand]*3.141592653589793/180;
        const double anchorSign=player.grip==0 ? 1 : -1;
        pose.anchors[a]={pose.wrists[hand].x-anchorSign*2*std::cos(angle),pose.wrists[hand].y-2*std::sin(angle)+(hand? -1:1),8};
        pose.anchors[b]={pose.wrists[hand].x+anchorSign*2*std::cos(angle),pose.wrists[hand].y+2*std::sin(angle)+(hand? 1:-1),8};
        pose.cost+=pose.openings[hand]*.15+std::abs(pose.rotations[hand])*.65+pose.reaches[hand]*.05;
        if (half >= player.shaft) issue(pose,"shaft",name+" hand's "+number(pose.openings[hand])+" cm span exceeds the configured shaft geometry.",2);
        if (pose.openings[hand]>player.opening) {
            issue(pose,hand?"right-opening":"left-opening",name+" opening "+number(pose.openings[hand])+" cm exceeds your "+number(player.opening)+" cm comfortable setting.",1);
            pose.cost+=3*(pose.openings[hand]-player.opening);
        }
        if (!targets.empty() && pose.reaches[hand]>player.reach) {
            issue(pose,hand?"right-reach":"left-reach",name+" reach "+number(pose.reaches[hand])+" cm exceeds your "+number(player.reach)+" cm profile; adjust the body position or instrument/player measurements.",player.calibrated?2:1);
            pose.cost+=3*(pose.reaches[hand]-player.reach);
        }
        if (std::abs(pose.rotations[hand])>player.rotation) {
            issue(pose,"rotation",name+" pair needs "+number(std::abs(pose.rotations[hand]),0)+"° of spread-plane tilt against your "+number(player.rotation,0)+"° comfort setting. A same-row pair or adjusted accidental strike can reduce it.",1);
            pose.cost+=std::abs(pose.rotations[hand])-player.rotation;
        }
    }
    pose.handClearance=distance(pose.wrists[0],pose.wrists[1])-player.handWidth;
    const bool both= (pose.active[0]||pose.active[1]) && (pose.active[2]||pose.active[3]);
    if (both && pose.handClearance<2) {
        issue(pose,"hands","Estimated hand clearance is "+number(pose.handClearance)+" cm. Crowded hands may require row separation, a wider opening or different body/strike positions.",1);
        pose.cost+=10*std::max(0.0,2-pose.handClearance);
    }
    if (pose.wrists[0].x>pose.wrists[1].x && both) {
        issue(pose,"arms","The hands cross left/right. This may be intentional; check preparation and arm clearance rather than assuming pitch overlap is a collision.",1); pose.cost+=12;
    }
    pose.shaftClearance=std::numeric_limits<double>::max();
    for (int a=0;a<4;++a) for (int b=a+1;b<4;++b) if (pose.active[a]&&pose.active[b]) {
        const double separation=distance(pose.targets[a],pose.targets[b]);
        if (separation<player.head) issue(pose,"heads","Mallets "+std::to_string(a+1)+" and "+std::to_string(b+1)+" overlap at the modeled strike points; adjust the points or attack.",2);
        if (a/2!=b/2) pose.shaftClearance=std::min(pose.shaftClearance,shaftDistance(pose.anchors[a],pose.targets[a],pose.anchors[b],pose.targets[b]));
    }
    if (pose.shaftClearance==std::numeric_limits<double>::max()) pose.shaftClearance=-1;
    if (pose.shaftClearance>=0 && pose.shaftClearance<1) {
        issue(pose,"clearance","Estimated inter-hand shaft clearance is "+number(pose.shaftClearance)+" cm. Motion and shaft height are not calibrated; verify this crossing before performance.",1);
        pose.cost+=15*(1-pose.shaftClearance);
    }
    if (pose.active[0]&&pose.active[3]) pose.outerSpread=distance(pose.targets[0],pose.targets[3]);
    transition(pose,previous,previousSeconds,player,"Previous");transition(pose,next,nextSeconds,player,"Next");
    if (pose.issues.empty()) issue(pose,"comfortable","No modeled range, order, clearance or comfort conflict. Keep this voicing; dimensions and motion remain estimates until calibrated.",0);
    return pose;
}
static Pose optimized(const Keyboard& k,const std::vector<int>& pitches,const std::vector<int>& ids,const Player& p,
                      const Pose* previous,double previousSeconds,const Pose* next,double nextSeconds) {
    auto fractions=p.strikeFractions;
    auto best=evaluate(k,pitches,ids,p,fractions,previous,previousSeconds,next,nextSeconds);
    if (!p.optimizeStrikes || best.severity == 0) return best;
    // One bounded coordinate pass: center versus front access. Manual strike
    // edits stay attached to source voices even when pitches change order.
    for (size_t i=0;i<pitches.size() && i<4;++i) {
        if (p.manualStrikes[i]) continue;
        const auto* b=k.bar(pitches[i]);if (!b) continue;
        for (const double f: b->accidental ? std::array<double,2>{.5,1-std::clamp((p.head*.5+.3)/b->length,.02,.25)} : std::array<double,2>{.45,.55}) {
            auto trial=best.fractions;trial[i]=f;
            auto candidate=evaluate(k,pitches,ids,p,trial,previous,previousSeconds,next,nextSeconds);
            // End strikes are a fallback, not a routine chord preference.
            // Require a changed warning level or a substantial modeled gain.
            if (better(candidate,best) && (candidate.severity < best.severity || best.cost-candidate.cost >= 8)) best=std::move(candidate);
        }
    }
    return best;
}
std::vector<Pose> solve(const Keyboard& k,const std::vector<int>& pitches,const Player& p,const std::vector<int>& required,
                        const Pose* previous,double previousSeconds,const Pose* next,double nextSeconds) {
    if (pitches.empty()) return {};
    if (pitches.size()>size_t(p.malletCount) || pitches.size()>4) {
        Pose pose;pose.pitches=pitches;issue(pose,"count","More independent simultaneous attacks than deployed mallets. A roll/arpeggiation is a different attack and must be explicitly chosen.",2);return {pose};
    }
    std::array<int,4> ids{1,2,3,4};std::set<std::vector<int>> seen;std::vector<Pose> out;
    do {
        std::vector<int> assignment(ids.begin(),ids.begin()+pitches.size());bool accepted=true;
        for (size_t i=0;i<assignment.size();++i) {
            const int id=assignment[i],r=i<required.size()?required[i]:0;
            if ((p.malletCount==2 && id!=1 && id!=4) || (p.malletCount==3 && id==3)
                || (r>0 && r!=id) || (r==-1 && id>2) || (r==-2 && id<3)) accepted=false;
        }
        if (accepted&&seen.insert(assignment).second) out.push_back(optimized(k,pitches,assignment,p,previous,previousSeconds,next,nextSeconds));
    } while(std::next_permutation(ids.begin(),ids.end()));
    std::stable_sort(out.begin(),out.end(),better);
    if (out.empty()) {Pose pose;pose.pitches=pitches;issue(pose,"sticking","Written sticking conflicts with simultaneous attacks or the deployed mallets. The known assignments have not been replaced.",2);out.push_back(pose);}
    return out;
}
std::vector<Pose> alternatives(const Keyboard& k,const std::vector<int>& pitches,const Player& p,const Pose* previous,
                              const std::vector<int>& required,const SearchOptions& options,double previousSeconds,
                              const Pose* next,double nextSeconds) {
    if (pitches.empty()||pitches.size()>size_t(p.malletCount)||pitches.size()>4) return {};
    const bool written=std::any_of(required.begin(),required.end(),[](int v){return v!=0;});
    Player baseline=p; baseline.optimizeStrikes=false;
    const auto original=solve(k,pitches,baseline,required,previous,previousSeconds,next,nextSeconds).front();
    std::vector<Pose> same,kept,changed;
    std::set<std::tuple<std::vector<int>,std::vector<int>,std::array<double,4>>> seen;
    seen.insert({original.pitches,original.mallets,original.fractions});
    auto append=[&](std::vector<Pose>& group,const Pose& pose) {
        if (pose.valid && seen.insert({pose.pitches,pose.mallets,pose.fractions}).second) group.push_back(pose);
    };
    if (written) {
        for (const auto& pose:solve(k,pitches,p,required,previous,previousSeconds,next,nextSeconds)) append(kept,pose);
    }
    for (const auto& pose:solve(k,pitches,p,{},previous,previousSeconds,next,nextSeconds)) {
        // With partial sticking, completions of the known constraints come
        // before reassignment; the original already is the best completion.
        append(same,pose);
    }
    std::vector<std::vector<int>> voicings;
    auto generate=[&](std::vector<int> v) {
        if (v==pitches) return;
        if (options.keepBass && v.front()!=pitches.front()) return;
        if (options.keepMelody && v.back()!=pitches.back()) return;
        if (options.keepBass && *std::min_element(v.begin(),v.end())!=pitches.front()) return;
        if (options.keepMelody && *std::max_element(v.begin(),v.end())!=pitches.back()) return;
        if (!options.allowInversion && *std::min_element(v.begin(),v.end())%12!=pitches.front()%12) return;
        for (int pitch:v) if (!k.bar(pitch)) return;
        auto sorted=v;std::sort(sorted.begin(),sorted.end());
        if (std::adjacent_find(sorted.begin(),sorted.end())!=sorted.end()) return;
        if (std::find(voicings.begin(),voicings.end(),v)==voicings.end()) voicings.push_back(std::move(v));
    };
    if (options.allowOctaves) {
        for (size_t n=0;n<pitches.size();++n) for (int d:{-24,-12,12,24}) {auto v=pitches;v[n]+=d;generate(v);}
        for (size_t a=0;a<pitches.size();++a) for (size_t b=a+1;b<pitches.size();++b)
            for (int x:{-12,12}) for(int y:{-12,12}) {auto v=pitches;v[a]+=x;v[b]+=y;generate(v);}
    }
    for (const auto& v:voicings) {
        if (written) {
            auto solutions=solve(k,v,p,required,previous,previousSeconds,next,nextSeconds);
            if (!solutions.empty()) append(kept,solutions.front());
        }
        auto solutions=solve(k,v,p,{},previous,previousSeconds,next,nextSeconds);
        if (!solutions.empty()) append(changed,solutions.front());
    }
    auto fidelity=[&](const Pose& pose) {int difference=0;for(size_t i=0;i<pitches.size();++i)difference+=std::abs(pose.pitches[i]-pitches[i]);return difference;};
    auto rank=[&](const Pose& a,const Pose& b) {
        if (a.severity!=b.severity) return a.severity<b.severity;
        const auto fa=fidelity(a),fb=fidelity(b);if(fa!=fb)return fa<fb;
        return better(a,b);
    };
    std::stable_sort(same.begin(),same.end(),better);std::stable_sort(kept.begin(),kept.end(),rank);std::stable_sort(changed.begin(),changed.end(),rank);
    std::vector<Pose> out;
    auto take=[&](const std::vector<Pose>& group,size_t max) {for(size_t i=0;i<std::min(max,group.size()) && out.size()<8;++i)out.push_back(group[i]);};
    // User's explicit update supersedes the earlier same-notes-first rule:
    // preserve written voice→mallet links before offering reassignment.
    if(written) {take(kept,3);take(same,3);take(changed,2);}else {take(same,3);take(changed,5);}
    return out;
}
}
