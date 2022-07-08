package com.coboltforge.dontmind.multivnc;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.EditText;
import android.widget.RadioGroup;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.Toast;

import com.google.android.material.switchmaterial.SwitchMaterial;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.util.Arrays;
import java.util.List;

/**
    Presents a UI to edit a {@link ConnectionBean}.
 */
public class ConnectionEditFragment extends Fragment {

    private static final String TAG = "ConnectionEditFragment";
    private static final int REQUEST_CODE_SSH_PRIVKEY_IMPORT = 11;

    private final String[] ENCODING_NAMES = {"Tight", "ZRLE", "Ultra", "Copyrect", "Hextile", "Zlib", "CoRRE", "RRE", "TRLE", "ZYWRLE"};
    private final String[] ENCODING_VALUES = {"tight", "zrle", "ultra", "copyrect", "hextile", "zlib", "corre", "rre", "trle", "zywrle"};

    private EditText bookmarkNameText;
    private EditText ipText;
    private EditText portText;
    private EditText passwordText;
    private TextView repeaterText;
    private Spinner colorSpinner;
    private boolean[] encodingChecks = new boolean[ENCODING_NAMES.length];
    private boolean[] encodingChecksEdit = new boolean[ENCODING_NAMES.length];
    private Spinner compressSpinner;
    private Spinner qualitySpinner;
    private EditText textUsername;
    private CheckBox checkboxKeepPassword;
    private EditText sshHostText;
    private EditText sshUsernameText;
    private EditText sshPasswordText;
    private Button sshPrivkeyImportButton;
    private byte[] sshPrivkey;
    private EditText sshPrivkeyPasswordText;

    // the represented Connection
    ConnectionBean conn = new ConnectionBean();

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        // Inflate the layout for this fragment
        return inflater.inflate(R.layout.connection_edit_fragment, container, false);
    }

    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        Log.d(TAG, "onViewCreated");

        bookmarkNameText = (EditText) view.findViewById(R.id.textNicknameBookmark);
        ipText = (EditText) view.findViewById(R.id.textIP);
        portText = (EditText) view.findViewById(R.id.textPORT);
        passwordText = (EditText) view.findViewById(R.id.textPASSWORD);
        textUsername = (EditText) view.findViewById(R.id.textUsername);

        colorSpinner = (Spinner)view.findViewById(R.id.spinnerColorMode);
        COLORMODEL[] models = {COLORMODEL.C24bit, COLORMODEL.C16bit};

        ArrayAdapter<COLORMODEL> colorSpinnerAdapter = new ArrayAdapter<>(getContext(), android.R.layout.simple_spinner_item, models);
        colorSpinner.setAdapter(colorSpinnerAdapter);

        AlertDialog.Builder encodingBuilder = new AlertDialog.Builder(requireContext());
        encodingBuilder.setTitle(R.string.encoding_caption)
                .setMultiChoiceItems(ENCODING_NAMES, encodingChecksEdit, new DialogInterface.OnMultiChoiceClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which, boolean isChecked)
                    {
                        encodingChecksEdit[which] = isChecked;
                    }
                })
                .setPositiveButton(R.string.encoding_ok, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        for (int i = 0; i < ENCODING_VALUES.length; ++i)
                        {
                            encodingChecks[i] = encodingChecksEdit[i];
                        }
                    }
                })
                .setNegativeButton(R.string.encoding_cancel, new DialogInterface.OnClickListener()
                {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        for (int i = 0; i < ENCODING_VALUES.length; ++i)
                        {
                            encodingChecksEdit[i] = encodingChecks[i];
                        }
                    }
                });
        Button encodingButton = (Button) view.findViewById(R.id.buttonEncoding);
        encodingButton.setOnClickListener(new View.OnClickListener()
        {
            @Override
            public void onClick(View v) {
                encodingBuilder.show();
            }
        });

        compressSpinner = (Spinner)view.findViewById(R.id.spinnerCompress);
        ArrayAdapter<COMPRESSMODEL> compressSpinnerAdapter = new ArrayAdapter<COMPRESSMODEL>(requireContext(), android.R.layout.simple_spinner_item, COMPRESSMODEL.values());
        compressSpinner.setAdapter(compressSpinnerAdapter);

        qualitySpinner = (Spinner)view.findViewById(R.id.spinnerQuality);
        ArrayAdapter<QUALITYMODEL> qualitySpinnerAdapter = new ArrayAdapter<QUALITYMODEL>(requireContext(), android.R.layout.simple_spinner_item, QUALITYMODEL.values());
        qualitySpinner.setAdapter(qualitySpinnerAdapter);

        checkboxKeepPassword = (CheckBox)view.findViewById(R.id.checkboxKeepPassword);

        repeaterText = (TextView)view.findViewById(R.id.textRepeaterId);

        sshHostText = view.findViewById(R.id.ssh_host_input);
        sshUsernameText = view.findViewById(R.id.ssh_username_input);
        sshPasswordText = view.findViewById(R.id.ssh_password_input);
        sshPrivkeyImportButton = view.findViewById(R.id.ssh_privkey_import_button);
        sshPrivkeyPasswordText = view.findViewById(R.id.ssh_privkey_password_input);
        SwitchMaterial sshSwitch = view.findViewById(R.id.ssh_switch);
        sshSwitch.setOnCheckedChangeListener((buttonView, isChecked) -> {
            // set visibility
            view.findViewById(R.id.ssh_row).setVisibility(isChecked ? View.VISIBLE : View.GONE);
            // and clear contents if disabled again
            if(!isChecked) {
                sshHostText.setText("");
                sshUsernameText.setText("");
                sshPasswordText.setText("");
                sshPrivkeyPasswordText.setText("");
            }
        });
        RadioGroup sshCredentialsRadioGroup = view.findViewById(R.id.ssh_credentials_radiogroup);
        sshCredentialsRadioGroup.setOnCheckedChangeListener((group, checkedId) -> {
            if(checkedId == R.id.ssh_password_radiobutton) {
                sshPasswordText.setVisibility(View.VISIBLE);
                sshPrivkeyImportButton.setVisibility(View.GONE);
                sshPrivkeyPasswordText.setVisibility(View.GONE);
                sshPrivkeyPasswordText.setText("");
                sshPrivkey = null;
            } else {
                sshPasswordText.setVisibility(View.GONE);
                sshPasswordText.setText("");
                sshPrivkeyImportButton.setVisibility(View.VISIBLE);
                sshPrivkeyPasswordText.setVisibility(View.VISIBLE);
            }
        });
        sshPrivkeyImportButton.setOnClickListener(v -> {
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE);
            intent.setType("*/*");
            startActivityForResult(intent, REQUEST_CODE_SSH_PRIVKEY_IMPORT);
        });

        // if we're editing a previously saved Connection, update UI accordingly
        updateViewsIfBookmarkedConnection(view);
    }

    @Override
    public void onDestroyView() {
        super.onDestroyView();
        Log.d(TAG, "onDestroyView");
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == REQUEST_CODE_SSH_PRIVKEY_IMPORT && resultCode == Activity.RESULT_OK) {
            if (data != null) {
                Uri uri = data.getData();
                try {
                    ByteArrayOutputStream byteBuffer = new ByteArrayOutputStream();
                    InputStream inputStream = requireContext().getContentResolver().openInputStream(uri);
                    int bufferSize = 4096;
                    byte[] buffer = new byte[bufferSize];
                    int len;
                    while ((len = inputStream.read(buffer)) != -1) {
                        byteBuffer.write(buffer, 0, len);
                    }
                    sshPrivkey = byteBuffer.toByteArray();
                    Toast.makeText(getContext(), R.string.ssh_privkey_import_success, Toast.LENGTH_LONG).show();
                } catch(Exception e) {
                    Toast.makeText(getContext(), R.string.ssh_privkey_import_fail, Toast.LENGTH_LONG).show();
                }

            }

        }
    }

    public ConnectionBean getConnection() {

        conn.address = ipText.getText().toString().trim();

        if(conn.address.length() == 0)
            return null;

        conn.nickname = bookmarkNameText.getText().toString();

        try {
            conn.port = Integer.parseInt(portText.getText().toString().trim());
        }
        catch (NumberFormatException ignored) {
        }
        conn.userName = textUsername.getText().toString().trim();
        conn.password = passwordText.getText().toString().trim();
        conn.keepPassword = checkboxKeepPassword.isChecked();
        conn.useLocalCursor = true; // always enable
        conn.colorModel = ((COLORMODEL)colorSpinner.getSelectedItem()).nameString();
        conn.compressModel = ((COMPRESSMODEL)compressSpinner.getSelectedItem()).nameString();
        conn.qualityModel = ((QUALITYMODEL)qualitySpinner.getSelectedItem()).nameString();
        if (repeaterText.getText().length() > 0)
        {
            conn.repeaterId = repeaterText.getText().toString().trim();
            conn.useRepeater = true;
        }
        else
        {
            conn.useRepeater = false;
        }
        conn.encodingsString = "";
        for (int i = 0; i < ENCODING_VALUES.length; ++i)
        {
            if (encodingChecks[i]) conn.encodingsString += ENCODING_VALUES[i] + " ";
        }
        conn.encodingsString += "raw";

        conn.sshHost = sshHostText.getText().toString().trim();
        if(conn.sshHost.isEmpty())
            conn.sshHost = null;
        conn.sshUsername = sshUsernameText.getText().toString().trim();
        if(conn.sshUsername.isEmpty())
            conn.sshUsername = null;
        conn.sshPassword = sshPasswordText.getText().toString().trim();
        if(conn.sshPassword.isEmpty())
            conn.sshPassword = null;
        conn.sshPrivkey = sshPrivkey;
        conn.sshPrivkeyPassword = sshPrivkeyPasswordText.getText().toString().trim();
        if(conn.sshPrivkeyPassword.isEmpty())
            conn.sshPrivkeyPassword = null;

        return conn;
    }

    public void setConnection(ConnectionBean conn) {
        this.conn = conn;
    }

    private <T extends Enum<T>> void setSpinnerByEnum(Spinner spinner, T[] values, T value) {
        for (int i=0; i<values.length; ++i)
            if (values[i] == value) {
                spinner.setSelection(i);
                break;
            }
    }

    private void updateViewsIfBookmarkedConnection(View view) {

        // if this is a connection that was not bookmarked, don't do anything
        if (conn.id == 0)
            return;

        view.findViewById(R.id.name_row).setVisibility(View.VISIBLE);

        bookmarkNameText.setText(conn.nickname);
        ipText.setText(conn.address);
        portText.setText(Integer.toString(conn.port));
        if (conn.keepPassword || conn.password.length()>0) {
            passwordText.setText(conn.password);
        }
        checkboxKeepPassword.setChecked(conn.keepPassword);
        textUsername.setText(conn.userName);

        setSpinnerByEnum(colorSpinner, COLORMODEL.values(), COLORMODEL.valueOf(conn.colorModel));
        setSpinnerByEnum(compressSpinner, COMPRESSMODEL.values(), COMPRESSMODEL.valueOf(conn.compressModel));
        setSpinnerByEnum(qualitySpinner, QUALITYMODEL.values(), QUALITYMODEL.valueOf(conn.qualityModel));

        if(conn.useRepeater)
            repeaterText.setText(conn.repeaterId);
        List<String> encodingValues = Arrays.asList(conn.encodingsString.split(" "));
        for (int i = 0; i < ENCODING_VALUES.length; ++i)
        {
            encodingChecksEdit[i] = encodingChecks[i] = encodingValues.contains(ENCODING_VALUES[i]);
        }
    }


}