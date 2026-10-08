# Mallet placement research — supplied source leads

The user supplied this bibliography from “Research mallet placement patterns” on October 8, 2026, followed by the complete 36-page **Keyboard Percussion and Multi-Mallet Placement: A Deep Research Synthesis.pdf**. The report's full extracted text was reviewed. This record filters duplicate and unrelated search results and identifies implementation implications; it is not an independently verified primary-source literature review. Book listings establish bibliographic leads, not access to their full technique chapters. Several copied snippets appear under mismatched titles; source text and authorship must be checked before attribution.

## Priority sources

| Supplied source | Intended use in the visualizer |
| --- | --- |
| Leigh Howard Stevens, [Method of Movement for Marimba: With 590 Exercises](https://books.google.com/books/about/Method_of_Movement_for_Marimba.html?id=pQnjAAAAMAAJ) | Investigate grip mechanics, stroke categories, interval changes, and efficient movement. |
| PAS, [Playing with Four Mallets: How to Hold Them](https://pas.org/pas-blog/playing-with-four-mallets-how-to-hold-them/) | Compare grip assumptions before judging a placement. |
| PAS, Playing with Four Mallets: Body Positioning | Investigate performer position and movement across the keyboard; the pasted excerpt alone is insufficient to set a fixed distance. |
| PAS, Playing with Four Mallets: Interval Changes | Investigate changes between successive notes/chords, not only static chord reach. |
| PAS, [Single Independent Strokes](https://pas.org/pas-blog/playing-with-four-mallets-single-independent-strokes/) and [Double Lateral Strokes](https://pas.org/pas-blog/playing-with-four-mallets-double-lateral-strokes/) | Investigate stroke types and mallet assignment over time. |
| Nancy Zeltsman, [Publications](https://nancyzeltsman.com/publications), including Four-Mallet Marimba Playing | Investigate musical sticking and phrasing alternatives. |
| Gary Burton, [Four Mallet Studies](https://www.steveweissmusic.com/products/gary-burton-four-mallet-studies) | Investigate crossed-grip assumptions and vibraphone-specific practice. |
| Mitchell Peters, [Fundamental Method for Mallets, Book 2](https://books.google.com/books/about/Fundamental_Method_for_Mallets.html?id=VWNQyWOkisYC) | Investigate general mallet technique and notation context. |
| Julia Gaines, [Sequential Studies, Book 1](https://tapspace.com/product/sequential-studies-book-1/) and [Book 2](https://tapspace.com/product/sequential-studies-book-2/) | Investigate progressive technique and transition difficulty. |
| Gifford Howarth, Simply Four; David Skidmore, A Fresh Approach to Technique and Musicianship with Four Mallets | Additional supplied technique leads; verify detailed mechanics rather than inferring them from publisher descriptions. |
| [Mallet Chord Studies](https://www.halleonard.com/product/6620134/mallet-chord-studies) and Juan Álamo, Marimbissimo | Investigate chord voicings and alternatives while making changed musical content explicit. |
| [Temporospatial Alterations in Upper-Limb and Mallet Control Underlie Motor Learning in Marimba Performance](https://pmc.ncbi.nlm.nih.gov/articles/PMC8866314/) | Investigate movement, timing, and learning variability; assess study scope before deriving a playback or diagnostic rule. |
| Yamaha, [Various playing techniques](https://www.yamaha.com/en/musical_instrument_guide/marimba/play/) | Investigate instrument-specific striking techniques and visual explanations. |
| Yamaha, [Fix It: Vibraphone Teaching Tips](https://hub.yamaha.com/music-educators/instruments/perc/vibraphone-pedagogy/); David Friedman, [Vibraphone Technique: Dampening and Pedaling](https://search.worldcat.org/title/Vibraphone-technique-%3A-dampening-and-pedaling/oclc/226205904) | Investigate vibraphone damping and pedal interaction separately from marimba technique. |

Other relevant titles in the supplied scanned-source list include Four-Mallet Sticking Options for Marimba, A Comparative Analysis of the Mechanics of Musser Grip, Stevens Grip, Cross Grip, and Burton Grip, Get a Grip: An Anatomical Survey of Four-Mallet Grips for Solo Marimba, Expanding Four-Mallet Marimba Techniques, and Gordon Stout's Ideo-Kinetics. Their full text and exact bibliographic details have not been verified here.

## Proposed rule design, pending source verification

Keep instrument-range checks distinct from player-dependent reach and comfort estimates. Interpret each placement in the context of grip, written sticking, performer dimensions, mallet geometry, strike location, and available time between events. A crossover can be a usable technique; its presence alone must not imply an unplayable chord. Evaluate transitions through a passage as well as individual chords.

Diagnostics should explain the selected assumptions and the specific geometric or timing issue. Alternative suggestions should distinguish keeping the original pitches with a new sticking/placement from changing register or voicing. Exact centimeter, angle, or speed thresholds need calibration and verified support. Defaults should be configurable, with estimated/model-dependent results labeled honestly.

## Access status

The uploaded PDF is available at `/workspace/attachments/f9a591d6-82b1-4d04-b83b-7eac897e6f03/Keyboard Percussion and Multi-Mallet Placement_ A Deep Research Synthesis.pdf`. It has 36 pages and identifies its author as ChatGPT Deep Research. Its full extracted text was read; the inaccessible cross-chat link is no longer needed to review this report.

Direct HTTPS requests to PAS grip/body/interval articles, the Yamaha technique guide, PMC8866314, and Nancy Zeltsman's publications page all returned `Tunnel connection failed: 403 Forbidden` from the environment proxy. These requests did not return article contents. Reading the uploaded synthesis does not establish independent verification of its primary citations.

## Translating the synthesis into reliable behavior

- **Keep physical mallet identities fixed.** The report defines M1–M4 by instantaneous pitch order (pages 1 and 14), with a later reassignment in its polychord example (pages 20–21). Software must instead keep hand ownership and physical mallet identity stable as shafts or arms cross. Offer an explicit sticking-numbering convention and translate imported markings; never silently renumber the avatar after sorting pitches.
- **Use contiguous 2+2 as a candidate, not a command.** Compare valid assignments against preceding/following events, written sticking, grip geometry, hand independence, and musical voices. Candidate scoring weights are adjustable heuristics; the report explicitly identifies its cost model and comparison rankings as synthesis, not measured laws.
- **Respect mallet counts within each hand.** The report's spread-voicing table suggests 1+3/3+1 partitions (page 16). A normal four-mallet setup with two mallets per hand cannot independently strike three distinct simultaneous targets with one hand. Such a proposal needs a different grip/count, an explicitly modeled special technique, or sequential attacks; do not silently offer it as an ordinary simultaneous four-mallet solution.
- **Count simultaneous new attacks separately from sustained sound.** On vibraphone, more than four pitches can remain ringing after earlier attacks. The report's discussion of six-note polychords (page 17) must not become a blanket rejection of six concurrently sustained notes. Respect note onset timing, pedal state, damping, rolls, and arpeggiation.
- **Use physical bar geometry, not the report's semitone coordinate as a ruler.** Its coordinate and chord diagrams are transposable/topological. Compute placement with graduated bar dimensions and row offsets; calibrated physical distances may vary by range/model. A musical fifth has different physical width in different registers.
- **Animate preparation and body movement.** The report emphasizes interval changes during recovery, moving the body with the active register, and preserving neutral wrist organization. Show those motion possibilities rather than assessing reach from a permanently stationary torso. Static pose estimates must not masquerade as verified human performance limits.
- **Distinguish articulation and instrument.** Free rebound, dead stroke, double vertical, single independent, single alternating, double lateral, one-hand rolls, ripple order, and vibraphone damping/pedal use are different actions. Written intent must control the chosen behavior; do not add arpeggiation, damping, or alter voicing merely to make a candidate look comfortable.
- **Preserve the musical result when proposing alternatives.** Label same-pitch sticking/position changes separately from register changes, inversion changes, note omissions, and other reharmonization. Audition/Compare should preview; only an explicit undoable Commit edits the score.
- **Calibrate and qualify limits.** The report warns that interval recommendations are relative, its ranking tables are analytical synthesis, and cited motion studies do not define one correct technique. Its approximate body-distance guidance is a starting point, not a fixed requirement for every performer. Do not invent universal reach, wrist-angle, or tempo limits.

The report's exercises and curricula are useful examples for future validation and technique vocabulary. They are reference material, not instructions to add beginner tours or to automatically insert exercises into the app.
