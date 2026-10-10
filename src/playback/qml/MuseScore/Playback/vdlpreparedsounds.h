/* SPDX-License-Identifier: GPL-3.0-only; MuseScore-Studio-CLA-applies */
#pragma once
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QUuid>
#include "audio/common/audiotypes.h"

namespace mu::playback::vdl {
inline bool isPrepared(const muse::audio::AudioResourceMeta& meta)
{
    return muse::audio::isResourceType(meta, muse::audio::AudioResourceType::VstPlugin)
        && !meta.attributeVal(u"evanVdlProfile").empty();
}
inline bool isKontakt(const muse::audio::AudioResourceMeta& meta)
{
    return muse::audio::isResourceType(meta, muse::audio::AudioResourceType::VstPlugin)
        && QString::fromStdString(meta.id).contains("kontakt", Qt::CaseInsensitive);
}
inline QString directory()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/evanscore/vdl-prepared";
}
inline QString fileName(const QString& id)
{
    // Only UUID filenames from the local index are accepted.
    return QUuid(id).isNull() ? QString() : directory() + '/' + id + ".state";
}
inline QVariantList entries()
{
    QSettings prefs;
    prefs.beginGroup("evanscore/vdl/prepared");
    QVariantList result;
    for (const auto& id : prefs.childGroups()) {
        if (fileName(id).isEmpty() || !QFile::exists(fileName(id))) continue;
        prefs.beginGroup(id);
        result << QVariantMap {{"id", id}, {"name", prefs.value("name")}, {"plugin", prefs.value("plugin")}};
        prefs.endGroup();
    }
    return result;
}
inline bool save(const muse::audio::AudioInputParams& params, QString name, bool snareManual, QString& error)
{
    name = name.trimmed();
    if (!isKontakt(params.resourceMeta) || name.isEmpty() || name.size() > 100) {
        error = QObject::tr("Choose Kontakt, load one VDL patch, and enter a name first."); return false;
    }
    const auto component = params.configuration.find("componentState");
    if (component == params.configuration.end() || component->second.empty()) {
        error = QObject::tr("Kontakt has not supplied its patch state yet. Load the patch and close its editor, then try again."); return false;
    }
    quint64 size = 0;
    for (const auto& pair : params.configuration) size += pair.first.size() + pair.second.size();
    constexpr quint64 limit = 64 * 1024 * 1024;
    if (size > limit || params.configuration.size() > 32) {
        error = QObject::tr("The Kontakt state exceeds the 64 MB profile limit. Prepare one instrument per channel."); return false;
    }
    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    QDir().mkpath(directory());
    QSaveFile file(fileName(id));
    if (!file.open(QIODevice::WriteOnly)) { error = file.errorString(); return false; }
    QDataStream stream(&file); stream.setVersion(QDataStream::Qt_6_0);
    stream << quint32(0x4556444c) << quint32(2) << QString::fromStdString(params.resourceMeta.id) << snareManual
           << quint32(params.configuration.size());
    for (const auto& [key, value] : params.configuration)
        stream << QByteArray(key.data(), int(key.size())) << QByteArray(value.data(), int(value.size()));
    if (stream.status() != QDataStream::Ok || !file.commit()) { error = file.errorString(); return false; }
    QSettings prefs;
    prefs.beginGroup("evanscore/vdl/prepared/" + id);
    prefs.setValue("name", name); prefs.setValue("plugin", QString::fromStdString(params.resourceMeta.id)); prefs.sync();
    if (prefs.status() != QSettings::NoError) {
        QFile::remove(fileName(id)); error = QObject::tr("Could not save the prepared sound index."); return false;
    }
    return true;
}
inline bool load(const QString& id, const muse::audio::AudioResourceMeta& installed,
                 muse::audio::AudioInputParams& result, QString& error)
{
    QFile file(fileName(id));
    if (file.fileName().isEmpty() || !file.open(QIODevice::ReadOnly) || file.size() > 64 * 1024 * 1024 + 8192) {
        error = QObject::tr("The prepared sound file is missing or exceeds its size limit."); return false;
    }
    QDataStream stream(&file); stream.setVersion(QDataStream::Qt_6_0);
    quint32 magic = 0, version = 0, count = 0; QString plugin; bool snareManual = false;
    stream >> magic >> version >> plugin >> snareManual >> count;
    if (magic != 0x4556444c || version != 2 || count > 32 || plugin != QString::fromStdString(installed.id) || !isKontakt(installed)) {
        error = QObject::tr("The prepared sound does not match this Kontakt installation."); return false;
    }
    muse::audio::AudioInputParams candidate; candidate.resourceMeta = installed;
    for (quint32 i = 0; i < count; ++i) {
        QByteArray key, value; stream >> key >> value;
        if (stream.status() != QDataStream::Ok || key.isEmpty() || key.size() > 256
            || !candidate.configuration.emplace(key.toStdString(), value.toStdString()).second) {
            error = QObject::tr("The prepared Kontakt state is invalid."); return false;
        }
    }
    if (stream.status() != QDataStream::Ok || !file.atEnd() || candidate.configuration["componentState"].empty()) {
        error = QObject::tr("The prepared Kontakt state is incomplete."); return false;
    }
    QSettings prefs; prefs.beginGroup("evanscore/vdl/prepared/" + id);
    candidate.resourceMeta.attributes[u"evanVdlProfile"] = muse::String::fromQString(id);
    candidate.resourceMeta.attributes[u"evanVdlName"] = muse::String::fromQString(prefs.value("name").toString());
    if (snareManual) candidate.resourceMeta.attributes[u"evanVdlMap"] = u"snare-manual";
    result = std::move(candidate); return true;
}
}
