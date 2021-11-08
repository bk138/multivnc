package com.coboltforge.dontmind.multivnc

import android.app.Activity
import android.app.AlertDialog
import android.app.Dialog
import android.content.Context
import android.content.Intent
import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteOpenHelper
import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.appcompat.app.AppCompatDialogFragment
import com.antlersoft.android.contentxml.SqliteElement
import com.antlersoft.android.contentxml.SqliteElement.ReplaceStrategy
import com.coboltforge.dontmind.multivnc.ImportExport.exportDatabase
import com.coboltforge.dontmind.multivnc.ImportExport.importDatabase
import org.xml.sax.SAXException
import java.io.IOException
import java.io.InputStreamReader
import java.io.OutputStreamWriter
import java.io.Writer
import java.lang.Exception
import java.net.MalformedURLException

class ImportExportDialog : AppCompatDialogFragment() {

    companion object {
        private const val TAG = "ImportExportDialog"
        private const val REQUEST_CODE_READ_FILE = 42
        private const val REQUEST_CODE_WRITE_FILE = 43
    }

    private lateinit var dbOpener: DbOpener
    private lateinit var vncDatabase: VncDatabase

    override fun onAttach(context: Context) {
        super.onAttach(context)
        // these need a valid context so are in onAttach()
        dbOpener = DbOpener(context)
        vncDatabase = VncDatabase.getInstance(context)
    }

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val dialog = AlertDialog.Builder(context)
            .setTitle(R.string.import_export_settings)
            .setItems(
                arrayOf(
                    getString(R.string.import_settings),
                    getString(R.string.export_settings)
                )
            ) { _, _ ->
                // doing nothing here because we define onclick listeners later on because the
                // default implementation adds a dismiss() after the listeners are executed,
                // causing the fragment to get destroyed before any onActivityResult could fire.
            }
            .create()

        // https://stackoverflow.com/questions/13746412/prevent-dialogfragment-from-dismissing-when-button-is-clicked
        dialog.setOnShowListener {
            dialog.listView.setOnItemClickListener { _, _, item, _ ->
                when (item) {
                    0 -> {
                        // import
                        val intent = Intent(Intent.ACTION_OPEN_DOCUMENT)
                        intent.addCategory(Intent.CATEGORY_OPENABLE)
                        val mimeTypes = arrayOf("text/xml", "application/xml", "application/json")
                        intent.type = "*/*"
                        intent.putExtra(Intent.EXTRA_MIME_TYPES, mimeTypes)
                        startActivityForResult(intent, REQUEST_CODE_READ_FILE)
                    }
                    1 -> {
                        // export
                        val intent = Intent(Intent.ACTION_CREATE_DOCUMENT)
                        intent.addCategory(Intent.CATEGORY_OPENABLE)
                        intent.type = "application/json"
                        intent.putExtra(Intent.EXTRA_TITLE, "MultiVNC-Export.json")
                        startActivityForResult(intent, REQUEST_CODE_WRITE_FILE)
                    }
                }
            }

        }
        return dialog
    }

    override fun onDestroy() {
        super.onDestroy()
        dbOpener.close()
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, resultData: Intent?) {
        if (resultCode == Activity.RESULT_OK && resultData != null && resultData.data != null && context != null) {

            if (requestCode == REQUEST_CODE_READ_FILE) {
                Log.d(
                    TAG,
                    "user opened for reading " + resultData.data + " with type " + context!!.contentResolver.getType(
                        resultData.data!!
                    )
                )
                try {
                    val reader =
                        InputStreamReader(context!!.contentResolver.openInputStream(resultData.data!!))
                    if (context!!.contentResolver.getType(resultData.data!!)!!.contains("xml")) {
                        SqliteElement.importXmlStreamToDb(
                            dbOpener.writableDatabase,
                            reader,
                            ReplaceStrategy.REPLACE_EXISTING
                        )
                    } else {
                        importDatabase(vncDatabase, reader)
                    }
                    Log.d(TAG, "import successful!")
                    Toast.makeText(context, android.R.string.ok, Toast.LENGTH_LONG).show()
                } catch (e: Exception) {
                    when (e) {
                        is MalformedURLException -> errorNotify(
                            getString(R.string.import_error_url),
                            e
                        )
                        is SAXException -> errorNotify(getString(R.string.import_error_xml), e)
                        else -> errorNotify(getString(R.string.import_error_io), e)
                    }
                }
            }

            if (requestCode == REQUEST_CODE_WRITE_FILE) {
                Log.d(TAG, "user opened for writing ${resultData.data}")
                try {
                    val writer: Writer =
                        OutputStreamWriter(context!!.contentResolver.openOutputStream(resultData.data!!))
                    exportDatabase(vncDatabase, writer)
                    writer.close()
                    Log.d(TAG, "export successful!")
                    Toast.makeText(context, android.R.string.ok, Toast.LENGTH_LONG).show()
                } catch (ioe: IOException) {
                    errorNotify("I/O Exception exporting config", ioe)
                }
            }

        } else {
            Log.e(TAG, "onActivityResult fail")
        }

        dismiss()
    }


    private fun errorNotify(msg: String, t: Throwable) {
        Log.e(TAG, msg, t)
        Utils.showErrorMessage(context, msg + ": <br/><br/>" + t.message)
    }

    /**
     * Room does not provide us access to SQLiteOpenHelper which is required by
     * XML importer. So we implement one here with essential features only.
     */
    internal class DbOpener(context: Context) :
        SQLiteOpenHelper(context, VncDatabase.NAME, null, VncDatabase.VERSION) {
        override fun onCreate(db: SQLiteDatabase) {
            //Nothing to do because creation is handled by VncDatabase
        }

        override fun onUpgrade(db: SQLiteDatabase, oldVersion: Int, newVersion: Int) {
            //Nothing to do because migration is handled by VncDatabase
        }
    }

}