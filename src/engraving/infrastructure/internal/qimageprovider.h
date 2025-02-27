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
#ifndef MU_DRAW_QIMAGEPROVIDER_H
#define MU_DRAW_QIMAGEPROVIDER_H

#include "infrastructure/draw/iimageprovider.h"

namespace mu::draw {
class QImageProvider : public IImageProvider
{
public:
    std::shared_ptr<Pixmap> createPixmap(const ByteArray& data) const override;
    std::shared_ptr<Pixmap> createPixmap(int w, int h, int dpm, const Color& color) const override;

    Pixmap scaled(const Pixmap& origin, const Size& s) const override;

    IPaintProviderPtr painterForImage(std::shared_ptr<Pixmap> pixmap) override;
    void saveAsPng(std::shared_ptr<Pixmap> px, io::IODevice* device) override;
    std::shared_ptr<Pixmap> pixmapFromQVariant(const QVariant& val) override;
};
}

#endif // MU_DRAW_QIMAGEPROVIDER_H
