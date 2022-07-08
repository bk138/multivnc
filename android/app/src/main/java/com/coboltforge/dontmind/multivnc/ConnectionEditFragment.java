package com.coboltforge.dontmind.multivnc;

import android.app.Activity;
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

/**
    Presents a UI to edit a {@link ConnectionBean}.
 */
public class ConnectionEditFragment extends Fragment {

    private static final String TAG = "ConnectionEditFragment";
    private static final int REQUEST_CODE_SSH_PRIVKEY_IMPORT = 11;

    private EditText ipText;
    private EditText portText;
    private EditText passwordText;
    private TextView repeaterText;
    private Spinner colorSpinner;
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

        ipText = (EditText) view.findViewById(R.id.textIP);
        portText = (EditText) view.findViewById(R.id.textPORT);
        passwordText = (EditText) view.findViewById(R.id.textPASSWORD);
        textUsername = (EditText) view.findViewById(R.id.textUsername);

        colorSpinner = (Spinner)view.findViewById(R.id.spinnerColorMode);
        COLORMODEL[] models = {COLORMODEL.C24bit, COLORMODEL.C16bit};

        ArrayAdapter<COLORMODEL> colorSpinnerAdapter = new ArrayAdapter<>(getContext(), android.R.layout.simple_spinner_item, models);
        colorSpinner.setAdapter(colorSpinnerAdapter);

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
        if (repeaterText.getText().length() > 0)
        {
            conn.repeaterId = repeaterText.getText().toString().trim();
            conn.useRepeater = true;
        }
        else
        {
            conn.useRepeater = false;
        }

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
}