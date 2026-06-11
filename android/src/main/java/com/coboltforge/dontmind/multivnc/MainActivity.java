import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.view.Display;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.Toast;

public class MainActivity extends Activity {
    // ...

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // ...
        menu.add("Show Available Screen Dimensions");
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if (item.getTitle().toString().equals("Show Available Screen Dimensions")) {
            Display display = getWindowManager().getDefaultDisplay();
            int width = display.getWidth();
            int height = display.getHeight();
            String dimensions = width + "x" + height;
            Toast.makeText(this, dimensions, Toast.LENGTH_SHORT).show();
        }
        return super.onOptionsItemSelected(item);
    }
}