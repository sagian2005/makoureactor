/****************************************************************************
 ** Makou Reactor Final Fantasy VII Field Script Editor
 ** Copyright (C) 2009-2022 Arzel Jérôme <myst6re@gmail.com>
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
#include "BackgroundEditor.h"
#include "core/field/BackgroundFile.h"
#include "core/field/BackgroundTilesIO.h"

BackgroundEditor::BackgroundEditor(QWidget *parent)
    : QWidget(parent), _backgroundFile(nullptr)
{
	_layersComboBox = new QComboBox(this);
	for (quint8 i = 0; i < 4; ++i) {
		_layersComboBox->addItem(tr("Layer %1").arg(i + 1));
	}
	_sectionsList = new QListWidget(this);
	_sectionsList->setUniformItemSizes(true);
	_tileCountWidthSpinBox = new QSpinBox(this);
	_tileCountWidthSpinBox->setRange(-MAX_TILE_DST, MAX_TILE_DST);
	_tileCountHeightSpinBox = new QSpinBox(this);
	_tileCountHeightSpinBox->setRange(-MAX_TILE_DST, MAX_TILE_DST);
	_zSpinBox = new QSpinBox(this);
	_zSpinBox->setRange(0, 4096);
	_backgroundLayerWidget = new ImageGridWidget(this);
	_tileWidget = new ImageGridWidget(this);
	_tileWidget->setCellSize(1);

	QHBoxLayout *layerEditorLayout = new QHBoxLayout();
	layerEditorLayout->addWidget(new QLabel(tr("Width (in tile unit):"), this));
	layerEditorLayout->addWidget(_tileCountWidthSpinBox);
	layerEditorLayout->addWidget(new QLabel(tr("Height (in tile unit):"), this));
	layerEditorLayout->addWidget(_tileCountHeightSpinBox);
	layerEditorLayout->addWidget(new QLabel(tr("Z:"), this));
	layerEditorLayout->addWidget(_zSpinBox);

	QGridLayout *layout = new QGridLayout(this);
	layout->addWidget(_layersComboBox, 0, 0);
	layout->addWidget(_sectionsList, 1, 0);
	layout->addLayout(layerEditorLayout, 0, 1);
	layout->addWidget(_backgroundLayerWidget, 1, 1);
	layout->addWidget(_tileWidget, 0, 2, 2, 1);
	layout->setColumnStretch(0, 1);
	layout->setColumnStretch(1, 2);
	layout->setColumnStretch(2, 1);
	layout->setRowStretch(1, 2);

	connect(_layersComboBox, &QComboBox::currentIndexChanged, this, &BackgroundEditor::updateCurrentLayer);
	connect(_sectionsList, &QListWidget::currentItemChanged, this, &BackgroundEditor::updateCurrentSection);
	connect(_zSpinBox, &QSpinBox::valueChanged, this, &BackgroundEditor::updateZ);
	connect(_backgroundLayerWidget, &ImageGridWidget::currentCellChanged, this, &BackgroundEditor::updateCurrentTile);
}

void BackgroundEditor::setSections(const QList<quint16> &sections)
{
	_sectionsList->clear();
	int i = 0;

	for (quint16 id: sections) {
		QListWidgetItem *item = new QListWidgetItem(tr("Section %1").arg(i++));
		item->setData(Qt::UserRole, id);
		_sectionsList->addItem(item);
	}
}

void BackgroundEditor::setBackgroundFile(BackgroundFile *backgroundFile)
{
	_backgroundFile = backgroundFile;

	updateCurrentLayer(_layersComboBox->currentIndex());
}

void BackgroundEditor::clear()
{
	_backgroundFile = nullptr;
	_sectionsList->blockSignals(true);
	_sectionsList->clear();
	_sectionsList->blockSignals(false);
}

int BackgroundEditor::currentSection() const
{
	QListWidgetItem *item = _sectionsList->currentItem();
	if (item == nullptr) {
		return -1;
	}

	return item->data(Qt::UserRole).toInt();
}

void BackgroundEditor::updateCurrentLayer(int layer)
{
	bool hasSections = layer == 1;

	_sectionsList->setEnabled(hasSections);

	if (hasSections && _sectionsList->currentItem() == nullptr) {
		_sectionsList->blockSignals(true);
		_sectionsList->setCurrentRow(0);
		_sectionsList->blockSignals(false);
	}

	updateImageLabel(layer, hasSections ? currentSection() : -1);
}

void BackgroundEditor::updateCurrentSection(QListWidgetItem *current, QListWidgetItem *previous)
{
	Q_UNUSED(previous)

	updateCurrentSection2(current != nullptr ? current->data(Qt::UserRole).toInt() : -1);
}

void BackgroundEditor::updateCurrentSection2(int section)
{
	updateImageLabel(_layersComboBox->currentIndex(), section);
}

void BackgroundEditor::updateImageLabel(int layer, int section)
{
	if (_backgroundFile == nullptr || !_backgroundFile->isOpen()) {
		return;
	}

	qint16 z[] = {-1, -1};
	bool layers[4] = {false, false, false, false};
	if (layer >= 0 && layer < 4) {
		layers[layer] = true;
	}
	QSet<quint16> sections;
	_zSpinBox->blockSignals(true);
	if (section >= 0) {
		sections.insert(quint16(section));
		if (_zSpinBox->value() != section) {
			_zSpinBox->setValue(section);
			_zSpinBox->setEnabled(true);
		}
	} else {
		if (_zSpinBox->value() != 0) {
			_zSpinBox->setValue(0);
			_zSpinBox->setEnabled(false);
		}
	}
	_zSpinBox->blockSignals(false);

	QImage background = _backgroundFile->openBackground(nullptr, z, layers, sections.isEmpty() ? nullptr : &sections);
	_backgroundLayerWidget->setPixmap(QPixmap::fromImage(background));

	QSize size = _backgroundFile->tiles().filter(nullptr, z, layers, sections.isEmpty() ? nullptr : &sections).area();
	quint8 tileSize = layer > 1 ? 32 : 16;
	_backgroundLayerWidget->setCellSize(tileSize);
	_tileCountWidthSpinBox->setValue(size.width() / tileSize);
	_tileCountHeightSpinBox->setValue(size.height() / tileSize);
}

void BackgroundEditor::updateZ(int z)
{
	if (_backgroundFile == nullptr) {
		return;
	}

	QListWidgetItem *item = _sectionsList->currentItem();
	int section = item->data(Qt::UserRole).toInt();

	if (section >= 0) {
		_backgroundFile->setZLayer1(quint16(section), quint16(z));
		item->setData(Qt::UserRole, z);
		updateImageLabel(1, z);
		emit modified();
	}
}

void BackgroundEditor::updateCurrentTile(const QPoint &point)
{
	_tileWidget->setPixmap(_backgroundLayerWidget->cellPixmap(point));
}
