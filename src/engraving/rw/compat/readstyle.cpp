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
#include "readstyle.h"

#include "style/defaultstyle.h"
#include "style/style.h"
#include "rw/xml.h"

#include "libmscore/masterscore.h"

#include "readchordlisthook.h"

#include "log.h"

using namespace mu;
using namespace mu::engraving::compat;
using namespace Ms;

static int readStyleDefaultsVersion(MasterScore* score, const ByteArray& scoreData, const QString& completeBaseName)
{
    XmlReader e(scoreData);
    e.setDocName(completeBaseName);

    while (!e.atEnd()) {
        e.readNext();
        if (e.name() == "defaultsVersion") {
            return e.readInt();
        }
    }

    return ReadStyleHook::styleDefaultByMscVersion(score->mscVersion());
}

ReadStyleHook::ReadStyleHook(Ms::Score* score, const ByteArray& scoreData, const QString& completeBaseName)
    : m_score(score), m_scoreData(scoreData), m_completeBaseName(completeBaseName)
{
}

int ReadStyleHook::styleDefaultByMscVersion(const int mscVer)
{
    constexpr int LEGACY_MSC_VERSION_V302 = 302;
    constexpr int LEGACY_MSC_VERSION_V3 = 301;
    constexpr int LEGACY_MSC_VERSION_V2 = 206;
    constexpr int LEGACY_MSC_VERSION_V1 = 114;

    if (mscVer > LEGACY_MSC_VERSION_V3 && mscVer < MSCVERSION) {
        return LEGACY_MSC_VERSION_V302;
    }
    if (mscVer > LEGACY_MSC_VERSION_V2 && mscVer <= LEGACY_MSC_VERSION_V3) {
        return LEGACY_MSC_VERSION_V3;
    }

    if (mscVer > LEGACY_MSC_VERSION_V1 && mscVer <= LEGACY_MSC_VERSION_V2) {
        return LEGACY_MSC_VERSION_V2;
    }

    if (mscVer <= LEGACY_MSC_VERSION_V1) {
        return LEGACY_MSC_VERSION_V1;
    }

    return MSCVERSION;
}

void ReadStyleHook::setupDefaultStyle()
{
    IF_ASSERT_FAILED(m_score) {
        return;
    }

    int defaultsVersion = -1;
    if (m_score->isMaster()) {
        defaultsVersion = readStyleDefaultsVersion(m_score->masterScore(), m_scoreData, m_completeBaseName);
    } else {
        defaultsVersion = m_score->masterScore()->style().defaultStyleVersion();
    }

    m_score->setStyle(DefaultStyle::resolveStyleDefaults(defaultsVersion));
    m_score->style().setDefaultStyleVersion(defaultsVersion);
}

void ReadStyleHook::setupDefaultStyle(Ms::Score* score)
{
    IF_ASSERT_FAILED(!score->isMaster()) {
        return;
    }

    int defaultsVersion = score->masterScore()->style().defaultStyleVersion();
    score->setStyle(DefaultStyle::resolveStyleDefaults(defaultsVersion));
    score->style().setDefaultStyleVersion(defaultsVersion);
}

void ReadStyleHook::readStyleTag(Ms::XmlReader& e)
{
    readStyleTag(m_score, e);
}

void ReadStyleHook::readStyleTag(Ms::Score* score, Ms::XmlReader& e)
{
    ReadChordListHook clhook(score);
    score->style().read(e, &clhook);
}

bool ReadStyleHook::readStyleProperties(Ms::MStyle* style, Ms::XmlReader& e)
{
    return style->readProperties(e);
}
