#!/usr/bin/env node
// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Writes src/Pieces.h from the Rill Sound lab's scores, so the browser and the
// device play the same pieces from one source. Also writes
// tests/score_vectors.h: every note time the lab's own engine produces for
// each part, which the C++ engine has to reproduce exactly.
//
//   node tools/export_scores.mjs [path/to/rill-sound]
import {writeFileSync} from 'node:fs';
import {resolve,dirname} from 'node:path';
import {fileURLToPath,pathToFileURL} from 'node:url';

const root=resolve(dirname(fileURLToPath(import.meta.url)),'..');
const sound=resolve(process.argv[2]??resolve(root,'../rill-sound'));
const {PIECES,stageAt,phaseSlope,buildOrder,audibleSteps}=await import(pathToFileURL(resolve(sound,'src/lab/phase-scores.js')));
import {readFileSync} from 'node:fs';
// The lab's Bach scores: a catalogue and one file per piece, in public/bach.
const bach=slug=>JSON.parse(readFileSync(resolve(sound,'public/bach',`${slug}.json`),'utf8'));
const CATALOG=JSON.parse(readFileSync(resolve(sound,'public/bach/catalog.json'),'utf8'));

// The pieces this firmware carries, in menu order, and the sounds it has.
// Process pieces come from phase-scores.js; composed ones from bach-scores.js,
// played on a harpsichord synthesized as the lab's is.
const SLUGS=['piano-phase','clapping'];
// Every Bach piece in the lab's catalogue, in its order and collections.
const COMPOSED=CATALOG.map(entry=>entry.slug);
const SAMPLES=['/samples/hand-clap-1.wav','/samples/hand-clap-2.wav'];
const VOICES={piano:'Voice::Piano',sample:'Voice::Sample'};
const MODES={play:'Mode::Play',rest:'Mode::Rest',build:'Mode::Build',reduce:'Mode::Reduce'};

const patterns=[],notes=[],orders=[];
function patternIndex(pattern){
 let index=patterns.findIndex(entry=>entry.source===pattern);
 if(index>=0)return index;
 if(pattern.some(step=>Array.isArray(step)))throw new Error('Chords are not supported on the device yet');
 const order=buildOrder(pattern);
 patterns.push({source:pattern,notes:notes.length,length:pattern.length,order:orders.length,orderLength:order.length});
 notes.push(...pattern.map(step=>step===null?-1:step));
 orders.push(...order);
 return patterns.length-1;
}
const text=value=>JSON.stringify(value??'');
// Full double precision: the tempo canon's timing depends on exact thirds and sixths.
const float=value=>{const s=String(Number(value));return s.includes('.')||s.includes('e')?s:`${s}.0`;};

const stages=[],parts=[],pieces=[],sections=[],noteEvents=[];
for(const slug of SLUGS){
 const piece=PIECES[slug];
 const partStart=parts.length,sectionStart=sections.length;
 for(const section of piece.sections??[])sections.push(`{${float(section.at)},${text(section.label)}}`);
 for(const part of piece.devices){
  if(!VOICES[part.voice])throw new Error(`${slug}: no device voice for ${part.voice}`);
  const stageStart=stages.length;
  for(const stage of part.stages){
   const pattern=patternIndex(stage.pattern??part.pattern??piece.pattern);
   stages.push(`{${float(stage.cycles)},${MODES[stage.mode]},${stage.from!==undefined},${float(stage.from??0)},${float(stage.to??0)},${stage.rate!==undefined},${float(stage.rate??1)},${!!stage.restart},${!!stage.once},${float(stage.slips??0)},${pattern},${text(stage.label)},${text(stage.main)},${text(stage.word)}}`);
  }
  const sample=part.sample?SAMPLES.indexOf(part.sample):-1;
  if(part.sample&&sample<0)throw new Error(`${slug}: sample ${part.sample} is not embedded`);
  parts.push(`{${text(part.name)},${VOICES[part.voice]},${sample},${part.transpose??0},${float(part.level??1.3)},${float(part.rate??1)},${text(part.main)},${stageStart},${part.stages.length},0,0}`);
 }
 // The collection names the composer, so these need no line of their own.
 pieces.push(`{${text(piece.title)},${text(piece.composer)},"","After Reich",0.0f,${Math.round(piece.period*1e6)},${piece.cycle??piece.pattern.length},${piece.featured??-1},${partStart},${piece.devices.length},${sectionStart},${(piece.sections??[]).length}}`);
}

for(const slug of COMPOSED){
 const score=bach(slug);
 const partStart=parts.length;
 for(const voice of score.voices){
  parts.push(`{${text(voice.name)},Voice::Harpsichord,-1,0,1.3,1.0,"",0,0,${noteEvents.length},${voice.notes.length}}`);
  for(const [start,length,midi] of voice.notes)noteEvents.push(`{${float(start)}f,${float(length)}f,${midi}}`);
 }
 const beatsPerBar=score.meter[0]*4/score.meter[1];
 // A composed piece's pulse is one beat, so its cycle is a single pulse.
 // The lab's shorter name where the full one will not fit the small screen.
 const title={'canon-augmentation':'Canon by Augmentation'}[slug]??score.title;
 pieces.push(`{${text(title)},"Johann Sebastian Bach",${text(`Bach, ${score.catalogue}`)},${text(score.collection)},${float(beatsPerBar)}f,${Math.round(60e6/score.bpm)},1,-1,${partStart},${score.voices.length},0,0}`);
}

const list=(items,per=1)=>items.map((item,i)=>(i%per?'':'\n  ')+item).join(',');
writeFileSync(resolve(root,'src/Pieces.h'),`// Copyright (c) 2026 Bruce Blay
// SPDX-License-Identifier: GPL-3.0-or-later
// Generated by tools/export_scores.mjs from rill-sound/src/lab/phase-scores.js.
// Do not hand-edit: change the lab's scores and export again.
#pragma once
#include "ScoreTypes.h"

namespace score {

inline constexpr int8_t notes[] = {${list(notes,32)}
};
inline constexpr uint16_t orders[] = {${list(orders,32)}
};
inline constexpr Pattern patterns[] = {${list(patterns.map(p=>`{${p.notes},${p.length},${p.order},${p.orderLength}}`))}
};
inline constexpr Stage stages[] = {${list(stages)}
};
inline constexpr NoteEvent noteEvents[] = {${list(noteEvents,8)}
};
inline constexpr Part parts[] = {${list(parts)}
};
inline constexpr Section sections[] = {${list(sections)}
};
inline constexpr Piece pieces[] = {${list(pieces)}
};
inline constexpr unsigned pieceCount = ${pieces.length};

}  // namespace score
`);

// The lab engine's note timing without audio, as in rill-sound's tests: when
// each note of a part sounds, in pulses from the start of the score.
function noteTimes(piece,part){
 const steps=piece.cycle??piece.pattern.length,hits=[];
 let next=null,stageIndex=-1,stageNote=0;
 for(let local=0;;local++){
  const where=stageAt(part.stages,local/steps,steps);
  if(where.done)return hits;
  const {stage}=where,pattern=stage.pattern??part.pattern??piece.pattern,length=pattern.length;
  if(stage.slips)throw new Error('Vectors cannot cover random slips');
  next??=local+Math.round(where.phase);
  if(where.index!==stageIndex){stageIndex=where.index;stageNote=next;}
  const speed=1+phaseSlope(stage,steps);
  for(;;){
   const due=local+(next-local-where.phase)/speed;
   if(due>=local+1-1e-9)break;
   const position=stage.restart?next-stageNote:next;
   const step=((position%length)+length)%length;
   const note=stage.once&&(position<0||position>=length)?null:pattern[step];
   next++;
   if(due<local-0.25||note===null)continue;
   if(!audibleSteps(pattern,where).has(step))continue;
   hits.push(`{${Math.max(local,due).toFixed(6)},${note}}`);
  }
 }
}
const vectors=[],spans=[];
for(const slug of SLUGS){
 const piece=PIECES[slug];
 for(const part of piece.devices){
  const hits=noteTimes(piece,part);
  spans.push(`{${vectors.length},${hits.length}}`);
  vectors.push(...hits);
 }
}
for(const slug of COMPOSED){
 for(const voice of bach(slug).voices){
  spans.push(`{${vectors.length},${voice.notes.length}}`);
  for(const [start,,midi] of voice.notes)vectors.push(`{${Math.fround(start).toFixed(6)},${midi}}`);
 }
}
writeFileSync(resolve(root,'tests/score_vectors.h'),`// Generated by tools/export_scores.mjs: note times from the lab's engine.
#pragma once
struct Hit { double time; int note; };
struct Span { unsigned start, count; };
static const Hit hits[] = {${list(vectors,8)}
};
// One span per part, in the order of score::parts.
static const Span spans[] = {${list(spans,8)}
};
`);
console.log(`Exported ${pieces.length} pieces, ${parts.length} parts, ${stages.length} stages, ${patterns.length} patterns; ${vectors.length} test notes.`);
