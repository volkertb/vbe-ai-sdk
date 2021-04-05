make play
if ERRORLEVEL,1 goto done
make testw
if ERRORLEVEL,1 goto done
make pmidi
if ERRORLEVEL,1 goto done
make note
if ERRORLEVEL,1 goto done
make setpref
if ERRORLEVEL,1 goto done
make volume
:done


