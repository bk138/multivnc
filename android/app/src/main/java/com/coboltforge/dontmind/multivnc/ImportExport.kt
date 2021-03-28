package com.coboltforge.dontmind.multivnc

import kotlinx.serialization.Serializable
import kotlinx.serialization.decodeFromString
import kotlinx.serialization.encodeToString
import kotlinx.serialization.json.Json
import java.io.Reader
import java.io.Writer

/**
 * Import/Export Utility
 */
object ImportExport {

    /**
     * Current version of [Container].
     */
    private const val VERSION = 1

    @Serializable
    private data class Container(
            val version: Int,
            val connections: List<ConnectionBean>,
            val metaKeys: List<MetaKeyBean>,
            val metaLists: List<MetaList>
    )

    private val serializer = Json {
        encodeDefaults = false
        ignoreUnknownKeys = true
        prettyPrint = true
    }

    /**
     * Exports database to given [writer].
     */
    fun exportDatabase(db: VncDatabase, writer: Writer) {
        val container = Container(
                VERSION,
                db.connectionDao.all,
                db.metaKeyDao.all,
                db.metaListDao.all
        )

        val json = serializer.encodeToString(container)

        writer.write(json)
    }

    /**
     * Reads JSON data from given [reader] and imports it into [db].
     */
    fun importDatabase(db: VncDatabase, reader: Reader) {
        val json = reader.readText()
        val container = serializer.decodeFromString<Container>(json)

        db.runInTransaction {
            container.connections.forEach {
                if (db.connectionDao.get(it.id) == null)
                    db.connectionDao.insert(it)
                else
                    db.connectionDao.update(it)
            }

            container.metaKeys.forEach {
                if (db.metaKeyDao.get(it.id) == null)
                    db.metaKeyDao.insert(it)
                else
                    db.metaKeyDao.update(it)
            }

            container.metaLists.forEach {
                if (db.metaListDao.get(it.id) == null)
                    db.metaListDao.insert(it)
                else
                    db.metaListDao.update(it)
            }
        }
    }
}