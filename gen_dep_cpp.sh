find . -name "*.cpp" -exec awk -f depend.awk -F \" {} \;