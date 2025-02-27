/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gracenotesrenderer.h"

#include "libmscore/chord.h"

using namespace mu::engraving;
using namespace mu::mpe;

const ArticulationTypeSet& GraceNotesRenderer::supportedTypes()
{
    static const mpe::ArticulationTypeSet types = {
        mpe::ArticulationType::Acciaccatura, mpe::ArticulationType::PostAppoggiatura,
        mpe::ArticulationType::PreAppoggiatura
    };

    return types;
}

void GraceNotesRenderer::doRender(const Ms::EngravingItem* item, const mpe::ArticulationType type, const RenderingContext& context,
                                  mpe::PlaybackEventList& result)
{
    const Ms::Chord* chord = Ms::toChord(item);

    IF_ASSERT_FAILED(chord) {
        return;
    }

    if (isPlacedBeforePrincipalNote(type)) {
        renderPrependedGraceNotes(chord, context, type, result);
    } else {
        renderAppendedGraceNotes(chord, context, type, result);
    }
}

bool GraceNotesRenderer::isPlacedBeforePrincipalNote(const mpe::ArticulationType type)
{
    if (type == ArticulationType::Acciaccatura || type == ArticulationType::PreAppoggiatura) {
        return true;
    }

    return false;
}

void GraceNotesRenderer::renderPrependedGraceNotes(const Ms::Chord* chord, const RenderingContext& context,
                                                   const mpe::ArticulationType type,
                                                   mpe::PlaybackEventList& result)
{
    std::vector<NominalNoteCtx> graceCtxList = graceNotesCtxList(chord->graceNotesBefore(), context);

    duration_t actualGraceNotesDuration = graceNotesMaxAvailableDuration(type, context, graceCtxList.size());
    timestamp_t graceNotesTimestampFrom = graceNotesStartTimestamp(type, actualGraceNotesDuration, context.nominalTimestamp);

    timestamp_t principalNotesTimestampFrom = principalNotesStartTimestamp(type, actualGraceNotesDuration, context.nominalTimestamp);
    duration_t totalPrincipalNotesDuration = principalNotesDuration(actualGraceNotesDuration, context.nominalDuration);

    buildGraceNoteEvents(std::move(graceCtxList), graceNotesTimestampFrom, type,
                         actualGraceNotesDuration, result);

    buildPrincipalNoteEvents(chord, context, type, totalPrincipalNotesDuration, principalNotesTimestampFrom, result);
}

void GraceNotesRenderer::renderAppendedGraceNotes(const Ms::Chord* chord, const RenderingContext& context, const mpe::ArticulationType type,
                                                  mpe::PlaybackEventList& result)
{
    std::vector<NominalNoteCtx> graceCtxList = graceNotesCtxList(chord->graceNotesAfter(), context);

    duration_t actualGraceNotesDuration = graceNotesMaxAvailableDuration(type, context, graceCtxList.size());
    timestamp_t graceNotesTimestampFrom = graceNotesStartTimestamp(type, actualGraceNotesDuration,
                                                                   context.nominalTimestamp + context.nominalDuration);

    timestamp_t principalNotesTimestampFrom = principalNotesStartTimestamp(type, actualGraceNotesDuration, context.nominalTimestamp);
    duration_t totalPrincipalNotesDuration = principalNotesDuration(actualGraceNotesDuration, context.nominalDuration);

    buildPrincipalNoteEvents(chord, context, type, totalPrincipalNotesDuration, principalNotesTimestampFrom, result);

    buildGraceNoteEvents(std::move(graceCtxList), graceNotesTimestampFrom, type,
                         actualGraceNotesDuration, result);
}

duration_t GraceNotesRenderer::graceNotesTotalDuration(const std::vector<NominalNoteCtx>& noteCtxList)
{
    duration_t result = 0;

    for (const NominalNoteCtx& noteCtx : noteCtxList) {
        result += noteCtx.duration;
    }

    return result;
}

float GraceNotesRenderer::graceNotesDurationRatio(const mpe::duration_t totalDuration, const duration_t maxAvailableDuration)
{
    float result = 1.f;

    if (totalDuration > maxAvailableDuration) {
        result = maxAvailableDuration / static_cast<float>(totalDuration);
    }

    return result;
}

std::vector<NominalNoteCtx> GraceNotesRenderer::graceNotesCtxList(const std::vector<Ms::Chord*>& graceChords,
                                                                  const RenderingContext& context)
{
    std::vector<NominalNoteCtx> result;

    for (const Ms::Chord* graceChord : graceChords) {
        for (const Ms::Note* graceNote : graceChord->notes()) {
            if (!isNotePlayable(graceNote)) {
                continue;
            }

            NominalNoteCtx noteCtx(graceNote, context);
            noteCtx.duration = durationFromTicks(context.beatsPerSecond.val, graceChord->durationTypeTicks().ticks());
            result.push_back(std::move(noteCtx));
        }
    }

    return result;
}

void GraceNotesRenderer::buildGraceNoteEvents(std::vector<NominalNoteCtx>&& noteCtxList, const timestamp_t timestampFrom,
                                              const ArticulationType type, const duration_t availableDuration,
                                              mpe::PlaybackEventList& result)
{
    for (size_t i = 0; i < noteCtxList.size(); ++i) {
        NominalNoteCtx& noteCtx = noteCtxList.at(i);
        noteCtx.duration = RealRound(availableDuration / static_cast<float>(noteCtxList.size()), 0);
        noteCtx.timestamp = timestampFrom + i * noteCtx.duration;

        updateArticulationBoundaries(type, noteCtx.timestamp, noteCtx.duration, noteCtx.chordCtx.commonArticulations);

        result.emplace_back(buildNoteEvent(std::move(noteCtx)));
    }
}

void GraceNotesRenderer::buildPrincipalNoteEvents(const Ms::Chord* chord, const RenderingContext& context, const ArticulationType type,
                                                  const mpe::duration_t duration,
                                                  const mpe::timestamp_t timestamp,
                                                  mpe::PlaybackEventList& result)
{
    for (const Ms::Note* note : chord->notes()) {
        if (!isNotePlayable(note)) {
            continue;
        }

        NominalNoteCtx noteCtx(note, context);
        noteCtx.duration = duration;
        noteCtx.timestamp = timestamp;

        updateArticulationBoundaries(type, noteCtx.timestamp, noteCtx.duration, noteCtx.chordCtx.commonArticulations);

        result.emplace_back(buildNoteEvent(std::move(noteCtx)));
    }
}

duration_t GraceNotesRenderer::graceNotesMaxAvailableDuration(const ArticulationType type, const RenderingContext& ctx,
                                                              const size_t graceNotesCount)
{
    duration_t halvedDuration = 0.5 * ctx.nominalDuration;
    duration_t twoThirdsDuration = (2 * ctx.nominalDuration) / 3;

    if (type == ArticulationType::PostAppoggiatura
        || type == ArticulationType::PreAppoggiatura) {
        if (ctx.timeSignatureFraction.isCompound() && ctx.nominalDurationTicks > QUAVER_TICKS) {
            return twoThirdsDuration;
        } else {
            return halvedDuration;
        }
    }

    duration_t minAcciacaturaDuration = durationFromTicks(ctx.beatsPerSecond.val, DEMISEMIQUAVER_TICKS / 2);

    return std::min(minAcciacaturaDuration * static_cast<duration_t>(graceNotesCount), halvedDuration);
}

timestamp_t GraceNotesRenderer::graceNotesStartTimestamp(const mpe::ArticulationType type, const mpe::duration_t availableDuration,
                                                         const mpe::timestamp_t& nominalTimestamp)
{
    if (type == ArticulationType::PostAppoggiatura) {
        return nominalTimestamp - availableDuration;
    }

    return nominalTimestamp;
}

timestamp_t GraceNotesRenderer::principalNotesStartTimestamp(const mpe::ArticulationType type, const mpe::duration_t graceNotesDuration,
                                                             const mpe::timestamp_t& nominalTimestamp)
{
    if (type == ArticulationType::PreAppoggiatura
        || type == ArticulationType::Acciaccatura) {
        return nominalTimestamp + graceNotesDuration;
    }

    return nominalTimestamp;
}

timestamp_t GraceNotesRenderer::principalNotesDuration(const mpe::duration_t graceNotesDuration,
                                                       const mpe::duration_t& nominalDuration)
{
    return nominalDuration - graceNotesDuration;
}
