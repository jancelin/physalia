<map version="freeplane 1.6.0">
<!--To view this file, download free mind mapping software Freeplane from http://freeplane.sourceforge.net -->
<node TEXT="Serveur Data GNSS" FOLDED="false" ID="ID_1287652560" CREATED="1625477245070" MODIFIED="1625493051146" STYLE="oval" VGAP_QUANTITY="0.0 pt">
<font SIZE="18"/>
<hook NAME="MapStyle">
    <properties edgeColorConfiguration="#808080ff,#ff0000ff,#0000ffff,#00ff00ff,#ff00ffff,#00ffffff,#7c0000ff,#00007cff,#007c00ff,#7c007cff,#007c7cff,#7c7c00ff" fit_to_viewport="false" show_note_icons="true" show_icon_for_attributes="true"/>

<map_styles>
<stylenode LOCALIZED_TEXT="styles.root_node" STYLE="oval" UNIFORM_SHAPE="true" VGAP_QUANTITY="24.0 pt">
<font SIZE="24"/>
<stylenode LOCALIZED_TEXT="styles.predefined" POSITION="right" STYLE="bubble">
<stylenode LOCALIZED_TEXT="default" ICON_SIZE="12.0 pt" COLOR="#000000" STYLE="fork">
<font NAME="SansSerif" SIZE="10" BOLD="false" ITALIC="false"/>
</stylenode>
<stylenode LOCALIZED_TEXT="defaultstyle.details"/>
<stylenode LOCALIZED_TEXT="defaultstyle.attributes">
<font SIZE="9"/>
</stylenode>
<stylenode LOCALIZED_TEXT="defaultstyle.note" COLOR="#000000" BACKGROUND_COLOR="#ffffff" TEXT_ALIGN="LEFT"/>
<stylenode LOCALIZED_TEXT="defaultstyle.floating">
<edge STYLE="hide_edge"/>
<cloud COLOR="#f0f0f0" SHAPE="ROUND_RECT"/>
</stylenode>
</stylenode>
<stylenode LOCALIZED_TEXT="styles.user-defined" POSITION="right" STYLE="bubble">
<stylenode LOCALIZED_TEXT="styles.topic" COLOR="#18898b" STYLE="fork">
<font NAME="Liberation Sans" SIZE="10" BOLD="true"/>
</stylenode>
<stylenode LOCALIZED_TEXT="styles.subtopic" COLOR="#cc3300" STYLE="fork">
<font NAME="Liberation Sans" SIZE="10" BOLD="true"/>
</stylenode>
<stylenode LOCALIZED_TEXT="styles.subsubtopic" COLOR="#669900">
<font NAME="Liberation Sans" SIZE="10" BOLD="true"/>
</stylenode>
<stylenode LOCALIZED_TEXT="styles.important">
<icon BUILTIN="yes"/>
</stylenode>
</stylenode>
<stylenode LOCALIZED_TEXT="styles.AutomaticLayout" POSITION="right" STYLE="bubble">
<stylenode LOCALIZED_TEXT="AutomaticLayout.level.root" COLOR="#000000" STYLE="oval" SHAPE_HORIZONTAL_MARGIN="10.0 pt" SHAPE_VERTICAL_MARGIN="10.0 pt">
<font SIZE="18"/>
</stylenode>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,1" COLOR="#0033ff">
<font SIZE="16"/>
</stylenode>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,2" COLOR="#00b439">
<font SIZE="14"/>
</stylenode>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,3" COLOR="#990000">
<font SIZE="12"/>
</stylenode>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,4" COLOR="#111111">
<font SIZE="10"/>
</stylenode>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,5"/>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,6"/>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,7"/>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,8"/>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,9"/>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,10"/>
<stylenode LOCALIZED_TEXT="AutomaticLayout.level,11"/>
</stylenode>
</stylenode>
</map_styles>
</hook>
<hook NAME="AutomaticEdgeColor" COUNTER="9" RULE="ON_BRANCH_CREATION"/>
<hook URI="Caster_files/ordinateur.png" SIZE="0.69933045" NAME="ExternalObject"/>
<node TEXT="PgAdmin4 :5051" POSITION="left" ID="ID_275402669" CREATED="1625477815616" MODIFIED="1625493037377" HGAP_QUANTITY="76.2499981448055 pt" VSHIFT_QUANTITY="101.24999698251494 pt">
<arrowlink SHAPE="CUBIC_CURVE" COLOR="#999999" WIDTH="2" TRANSPARENCY="200" FONT_SIZE="9" FONT_FAMILY="SansSerif" DESTINATION="ID_120266587" MIDDLE_LABEL=" DB Administration" STARTINCLINATION="12;-18;" ENDINCLINATION="-110;16;" STARTARROW="NONE" ENDARROW="DEFAULT"/>
<edge COLOR="#00ffff"/>
<hook URI="Caster_files/pgadmin4.png" SIZE="0.45870903" NAME="ExternalObject"/>
</node>
<node TEXT="Docker / Docker-compose" POSITION="left" ID="ID_434452615" CREATED="1625489338384" MODIFIED="1625493051145" HGAP_QUANTITY="42.49999915063382 pt" VSHIFT_QUANTITY="195.7499941661956 pt">
<edge COLOR="#00007c"/>
<hook URI="Caster_files/docker.png" SIZE="0.7216004" NAME="ExternalObject"/>
<hook NAME="FreeNode"/>
</node>
<node TEXT="Get data from buoy :8090" POSITION="right" ID="ID_594473272" CREATED="1625477561213" MODIFIED="1625492479110" HGAP_QUANTITY="-389.49998797476326 pt" VSHIFT_QUANTITY="-178.49999468028562 pt">
<arrowlink SHAPE="CUBIC_CURVE" COLOR="#00cc00" WIDTH="2" TRANSPARENCY="200" FONT_SIZE="9" FONT_FAMILY="SansSerif" DESTINATION="ID_565231140" MIDDLE_LABEL="Get data" STARTINCLINATION="-40;-147;" ENDINCLINATION="-131;-5;" STARTARROW="DEFAULT" ENDARROW="NONE"/>
<edge COLOR="#0000ff"/>
<font SIZE="10"/>
<node TEXT="(Telegraf)" ID="ID_1928115556" CREATED="1625491209381" MODIFIED="1625492145244" HGAP_QUANTITY="-261.99999177455925 pt" VSHIFT_QUANTITY="11.24999966472388 pt" COLOR="#999999">
<edge COLOR="#c2aac2"/>
<hook URI="Caster_files/telegraf.png" SIZE="0.31298503" NAME="ExternalObject"/>
</node>
<node TEXT="TCP Serveur" ID="ID_423687682" CREATED="1625477465953" MODIFIED="1625492152587" HGAP_QUANTITY="-210.99999329447766 pt" VSHIFT_QUANTITY="20.24999939650299 pt">
<arrowlink SHAPE="CUBIC_CURVE" COLOR="#00cc00" WIDTH="2" TRANSPARENCY="200" FONT_SIZE="9" FONT_FAMILY="SansSerif" DESTINATION="ID_120266587" MIDDLE_LABEL="Store data" STARTINCLINATION="64;7;" ENDINCLINATION="30;-88;" STARTARROW="NONE" ENDARROW="DEFAULT"/>
<hook URI="reseau_files/python.png" SIZE="0.06583333" NAME="ExternalObject"/>
</node>
</node>
<node TEXT="WEB" POSITION="right" ID="ID_1640441052" CREATED="1625477412185" MODIFIED="1625493236032" HGAP_QUANTITY="252.49999289214625 pt" VSHIFT_QUANTITY="-79.49999763071543 pt">
<edge COLOR="#0000ff"/>
<hook URI="BaseRtk_files/internet.png" SIZE="0.725933" NAME="ExternalObject"/>
<hook NAME="FreeNode"/>
<node TEXT="httsp://IP:8090 &lt;&lt;&lt;" ID="ID_565231140" CREATED="1625478632055" MODIFIED="1625494856924" HGAP_QUANTITY="12.50000004470349 pt" VSHIFT_QUANTITY="-89.99999731779106 pt">
<node TEXT="" ID="ID_1779754199" CREATED="1625494707082" MODIFIED="1625494856923" HGAP_QUANTITY="141.499996200204 pt" VSHIFT_QUANTITY="-38.99999883770946 pt">
<edge COLOR="#ffffff"/>
<hook URI="rover_files/2-3-4g.png" SIZE="0.107421875" NAME="ExternalObject"/>
</node>
<node TEXT="" ID="ID_374566022" CREATED="1625493674465" MODIFIED="1625494865340" HGAP_QUANTITY="252.4999928921463 pt" VSHIFT_QUANTITY="-47.24999859184031 pt">
<edge COLOR="#ffffff" DASH="CLOSE_DOTS"/>
<hook URI="rover_files/physalia_carre_200px.png" SIZE="0.48" NAME="ExternalObject"/>
<hook NAME="FreeNode"/>
</node>
<node TEXT="" ID="ID_1882817828" CREATED="1625493674465" MODIFIED="1625494882504" HGAP_QUANTITY="175.24999519437566 pt" VSHIFT_QUANTITY="-44.249998681247256 pt">
<arrowlink SHAPE="CUBIC_CURVE" COLOR="#00cc00" WIDTH="2" TRANSPARENCY="200" FONT_SIZE="9" FONT_FAMILY="SansSerif" DESTINATION="ID_565231140" MIDDLE_LABEL="Send data" STARTINCLINATION="75;-14;" ENDINCLINATION="177;-152;" STARTARROW="NONE" ENDARROW="DEFAULT"/>
<edge COLOR="#ffffff" DASH="CLOSE_DOTS"/>
<hook URI="rover_files/physalia_carre_200px.png" SIZE="0.53" NAME="ExternalObject"/>
<hook NAME="FreeNode"/>
</node>
<node TEXT="" ID="ID_1361806712" CREATED="1625493674465" MODIFIED="1625494829038" HGAP_QUANTITY="322.99999079108267 pt" VSHIFT_QUANTITY="-42.749998725950775 pt">
<edge COLOR="#ffffff" DASH="CLOSE_DOTS"/>
<hook URI="rover_files/physalia_carre_200px.png" SIZE="0.345" NAME="ExternalObject"/>
<hook NAME="FreeNode"/>
</node>
</node>
<node TEXT="postgis:5433 &gt;&gt;&gt;" ID="ID_143670887" CREATED="1625492517953" MODIFIED="1625494798817" HGAP_QUANTITY="94.99999758601196 pt" VSHIFT_QUANTITY="0.7499999776482589 pt">
<arrowlink SHAPE="CUBIC_CURVE" COLOR="#33cc00" WIDTH="2" TRANSPARENCY="200" FONT_SIZE="9" FONT_FAMILY="SansSerif" DESTINATION="ID_623128404" STARTINCLINATION="61;88;" ENDINCLINATION="-140;-16;" STARTARROW="DEFAULT" ENDARROW="NONE"/>
<hook NAME="FreeNode"/>
<node TEXT="" ID="ID_623128404" CREATED="1625494063052" MODIFIED="1625494164513" HGAP_QUANTITY="180.4999950379135 pt" VSHIFT_QUANTITY="39.74999881535772 pt">
<edge COLOR="#ffffff"/>
<hook URI="Caster_files/qgis.png" SIZE="0.37146586" NAME="ExternalObject"/>
</node>
</node>
<node TEXT="https://IP:3000 &gt;&gt;&gt;" ID="ID_1787392185" CREATED="1625478678207" MODIFIED="1625494283843" HGAP_QUANTITY="16.99999991059304 pt" VSHIFT_QUANTITY="26.999999195337338 pt">
<node TEXT="" ID="ID_828136935" CREATED="1625478104157" MODIFIED="1625494283843" HGAP_QUANTITY="-161.4999947696926 pt" VSHIFT_QUANTITY="149.24999555200355 pt">
<hook URI="Caster_files/grafana_buoy.png" SIZE="0.86138016" NAME="ExternalObject"/>
</node>
</node>
</node>
<node TEXT="Database: Postgresql / Postgis :5433" POSITION="right" ID="ID_120266587" CREATED="1625477609292" MODIFIED="1625494240976" HGAP_QUANTITY="-207.99999338388466 pt" VSHIFT_QUANTITY="18.74999944120647 pt">
<arrowlink SHAPE="CUBIC_CURVE" COLOR="#00cc33" WIDTH="2" TRANSPARENCY="200" FONT_SIZE="9" FONT_FAMILY="SansSerif" DESTINATION="ID_143670887" MIDDLE_LABEL="Read data" STARTINCLINATION="76;-79;" ENDINCLINATION="-134;-126;" STARTARROW="DEFAULT" ENDARROW="NONE"/>
<edge COLOR="#0033cc"/>
<hook URI="Caster_files/postgis.png" SIZE="0.45" NAME="ExternalObject"/>
<hook NAME="FreeNode"/>
</node>
<node TEXT="Visualisation: Grafana :3000" POSITION="right" ID="ID_1445004074" CREATED="1625478025050" MODIFIED="1625493046960" HGAP_QUANTITY="-234.24999260157367 pt" VSHIFT_QUANTITY="254.99999240040802 pt">
<arrowlink SHAPE="CUBIC_CURVE" COLOR="#00cc33" WIDTH="2" TRANSPARENCY="200" FONT_SIZE="9" FONT_FAMILY="SansSerif" DESTINATION="ID_120266587" MIDDLE_LABEL="Read data" STARTINCLINATION="-15;-16;" ENDINCLINATION="-200;-38;" STARTARROW="NONE" ENDARROW="DEFAULT"/>
<arrowlink SHAPE="CUBIC_CURVE" COLOR="#00cc33" WIDTH="2" TRANSPARENCY="200" FONT_SIZE="9" FONT_FAMILY="SansSerif" DESTINATION="ID_828136935" MIDDLE_LABEL="View data" STARTINCLINATION="128;32;" ENDINCLINATION="-186;249;" STARTARROW="NONE" ENDARROW="DEFAULT"/>
<edge COLOR="#7c0000"/>
<hook URI="Caster_files/grafana.jpg" SIZE="0.18331693" NAME="ExternalObject"/>
<hook NAME="FreeNode"/>
</node>
</node>
</map>
