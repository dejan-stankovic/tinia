/* Copyright STIFTELSEN SINTEF 2012
 *
 * This file is part of the Tinia Framework.
 *
 * The Tinia Framework is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * The Tinia Framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with the Tinia Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

//#include <algorithm>
//#include <stdexcept>

#include <tinia/utils/ProxyDebugGUI.hpp>




namespace {

    // NB! This list must match the one in ProxyRenderer.js, exactly!
    const char *allowed_auto_proxy_algos[] = { "0) AngleCoverage-5",
                                               "1) AngleCoverage-2",
                                               "2) OnlyMostRecent",
                                               "3) ReplOldestWhnDiff-5",
                                               "4) ReplaceOldest-5",
                                               NULL };

}




namespace tinia {
namespace utils {




ProxyDebugGUI::ProxyDebugGUI( boost::shared_ptr<model::ExposedModel> model,
                              const bool with_ap, const bool with_ap_debugging, const bool with_jpg, const bool with_auto_select,
                              const bool with_depth_buffer_manipulation /* = false */ )
    : m_w_ap(with_ap), m_w_apd(with_ap_debugging), m_w_jpg(with_jpg), m_w_as(with_auto_select),
      m_with_depth_buffer_manipulation(with_depth_buffer_manipulation)
{
    if ( m_w_ap && m_w_jpg && m_w_as ) {
        model->addElement<bool>( "ap_autoSelect", false );            // Selects whatever proxy method works fastest.
        model->addElement<bool>( "ap_autoSelectSampleAll", false );
        model->addAnnotation("ap_autoSelectSampleAll", "Sample all");
        model->addElement( "ap_autoSelectIndicator", "---" );
        model->addConstrainedElement<int>("ap_autoSelectTargetTime", 100, 0, 200);
        model->addAnnotation("ap_autoSelectTargetTime", "Target time:");
        model->addConstrainedElement<int>("ap_autoSelectTargetTimeSlack", 20, 0, 200);
        model->addAnnotation("ap_autoSelectTargetTimeSlack", "Target time slack:");
        model->addConstrainedElement<int>("simulatedAdditionalLatency", 0, 0, 1000);
        model->addAnnotation("simulatedAdditionalLatency", "ms per frame:");
        model->addConstrainedElement<int>("simulatedAdditionalLatencyDecay", 0, -100, 100);
        model->addAnnotation("simulatedAdditionalLatencyDecay", "ms per sec:");
    }

    if (m_w_jpg) {
        model->addElement<bool>( "ap_useJpgProxy", false );            // This turns on the new "proxy mode", for which speedier jpg-snapshots are used
        model->addConstrainedElement<int>("ap_jpgQuality", 0, 0, 99);
        model->addAnnotation("ap_jpgQuality", "Jpg compression q:");
    }

    if (m_w_ap) {
        model->addElement<bool>( "ap_useAutoProxy", false );          // This turns on the new autoProxy. It will override useJpgProxy.
        model->addElement<bool>( "ap_autoProxyDebugging", true );
        model->addAnnotation("ap_autoProxyDebugging", "Debug mode");
    }

    if (m_w_ap && m_w_apd) {
        int algos=0;
        while ( allowed_auto_proxy_algos[algos] != NULL ) {
            algos++;
        }
        model->addElementWithRestriction<std::string>( "ap_autoProxyAlgo", allowed_auto_proxy_algos[2], &allowed_auto_proxy_algos[0], &allowed_auto_proxy_algos[0]+algos );
        model->addAnnotation("ap_autoProxyAlgo", "Proxy replacement algo");
        model->addElement<bool>( "ap_debugSplatCol", false );
        model->addAnnotation("ap_debugSplatCol", "Index coloring"); //  (r, g, b, y, c, m)");
        model->addElement<bool>( "ap_decayMode", false );
        model->addAnnotation("ap_decayMode", "Decaying splats");
        model->addElement<bool>( "ap_roundSplats", false );
        model->addAnnotation("ap_roundSplats", "Circular splats");
        model->addElement<bool>( "ap_screenSpaceSized", false );
        model->addAnnotation("ap_screenSpaceSized", "Screen-space-sized splats");
        model->addConstrainedElement<int>("ap_overlap", 100, 1, 300);
        model->addAnnotation("ap_overlap", "Overlap factor)");
        model->addElement<bool>( "ap_alwaysShowMostRecent", true );
        model->addAnnotation("ap_alwaysShowMostRecent", "Most recent model in front");
        model->addConstrainedElement<int>("ap_splats", 12, 2, 512);
        model->addAnnotation("ap_splats", "Number of splats)");
        model->addElement<bool>( "ap_resetAllModels", false );
        model->addAnnotation("ap_resetAllModels", "Reset and fix proxy");
        model->addElement<bool>( "ap_useISTC", true );
        model->addAnnotation("ap_useISTC", "Use intra-splat texcoo");
        model->addElement<bool>( "ap_splatOutline", true );
        model->addAnnotation("ap_splatOutline", "Square splat outline");
        model->addElement<bool>( "ap_reloadShader", false );
        model->addAnnotation("ap_reloadShader", "Reload shader");
        model->addElement<bool>( "ap_useFragExt", true );
        model->addAnnotation("ap_useFragExt", "Use FragDepthExt if available");
        model->addElement( "ap_fragExtStatus", "---" );
        model->addElement( "ap_consoleLog", "---" );
        model->addElement<int>( "ap_cntr", 0 );
    }

    if (with_depth_buffer_manipulation) {
        // To avoid confusion, it is wise to use as default value for these sliders, the same value as the canvas size in Canvas.js...
        model->addConstrainedElement<int>("ap_depthWidth", 1024, 16, 1024);
        model->addAnnotation("ap_depthWidth", "Depth buf width)");
        model->addConstrainedElement<int>("ap_depthHeight", 1024, 16, 1024);
        model->addAnnotation("ap_depthHeight", "Depth buf height)");
        model->addElement<bool>( "ap_mid_texel_sampling", false );              // false best
        model->addAnnotation("ap_mid_texel_sampling", "Sample mid-texel");
        model->addElement<bool>( "ap_use_qt_img_scaling", true );
        model->addAnnotation("ap_use_qt_img_scaling", "Qt-scaling (qtc)");
        model->addElement<bool>( "ap_set_depth_size_32", false );
        model->addAnnotation("ap_set_depth_size_32", "32");
        model->addElement<bool>( "ap_set_depth_size_64", false );
        model->addAnnotation("ap_set_depth_size_64", "64");
        model->addElement<bool>( "ap_set_depth_size_128", false );
        model->addAnnotation("ap_set_depth_size_128", "128");
        model->addElement<bool>( "ap_set_depth_size_256", false );
        model->addAnnotation("ap_set_depth_size_256", "256");
        model->addElement<bool>( "ap_set_depth_size_512", false );
        model->addAnnotation("ap_set_depth_size_512", "512");
        model->addElement<bool>( "ap_small_delta_sampling", true );             // true best
        model->addAnnotation("ap_small_delta_sampling", "Small delta");
        model->addElement<bool>( "ap_larger_delta_sampling", false );           // cannot see any difference
        model->addAnnotation("ap_larger_delta_sampling", "Larger delta");
        model->addElement<bool>( "ap_mid_splat_sampling", false );
        model->addAnnotation("ap_mid_splat_sampling", "Sample mid-splat");
        model->addElement<bool>( "ap_simulate_downsampling", false );
        model->addAnnotation("ap_simulate_downsampling", "Simul. downs (qtc)");
        model->addElement<bool>( "ap_bi_linear_filtering", false );
        model->addAnnotation("ap_bi_linear_filtering", "Bi-linear (qtc)");
        model->addElement<bool>( "ap_16_bit_depth", false );
        model->addAnnotation("ap_16_bit_depth", "16 bit depth");
        model->addElement<bool>( "ap_hold_up_png", false );
        model->addAnnotation("ap_hold_up_png", "Hold up PNG");
        model->addElement<bool>( "ap_whitebg", false );
        model->addAnnotation("ap_whitebg", "White bg");
        model->addElement<bool>( "ap_greenbg", false );
        model->addAnnotation("ap_greenbg", "Green bg");
        model->addElement<bool>( "ap_blackbg", false );
        model->addAnnotation("ap_blackbg", "Black bg");
        model->addElement<bool>( "ap_dump", false );
        model->addAnnotation("ap_dump", "Dump rgb (trell)");
        model->addElement<bool>( "ap_dump_button", false );
        model->addAnnotation("ap_dump_button", "Dump (js)");
    }
}




tinia::model::gui::Grid *ProxyDebugGUI::getGrid()
{
    tinia::model::gui::Grid *mainGrid = new tinia::model::gui::Grid(100, 5);
    int row = 0;

    if ( m_w_ap && m_w_jpg && m_w_as ) {
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("ap_autoSelect"));
        mainGrid->setChild(row, 1, new tinia::model::gui::Button("ap_autoSelectSampleAll"));
        //        mainGrid->setChild(row, 0, new tinia::model::gui::Label("ap_autoSelectIndicator", false)); // Showing the name of the label
        mainGrid->setChild(row, 2, new tinia::model::gui::Label("ap_autoSelectIndicator", true)); // Showing the content of the label
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::HorizontalSlider("ap_autoSelectTargetTime"));
        mainGrid->setChild(row, 1, new tinia::model::gui::Label("ap_autoSelectTargetTime", false));
        mainGrid->setChild(row, 2, new tinia::model::gui::Label("ap_autoSelectTargetTime", true));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::HorizontalSlider("ap_autoSelectTargetTimeSlack"));
        mainGrid->setChild(row, 1, new tinia::model::gui::Label("ap_autoSelectTargetTimeSlack", false));
        mainGrid->setChild(row, 2, new tinia::model::gui::Label("ap_autoSelectTargetTimeSlack", true));
        row++;

        mainGrid->setChild(row, 0, new tinia::model::gui::HorizontalSlider("simulatedAdditionalLatency"));
        mainGrid->setChild(row, 1, new tinia::model::gui::Label("simulatedAdditionalLatency", false));
        mainGrid->setChild(row, 2, new tinia::model::gui::Label("simulatedAdditionalLatency", true));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::HorizontalSlider("simulatedAdditionalLatencyDecay"));
        mainGrid->setChild(row, 1, new tinia::model::gui::Label("simulatedAdditionalLatencyDecay", false));
        mainGrid->setChild(row, 2, new tinia::model::gui::Label("simulatedAdditionalLatencyDecay", true));
        row++;
    }

    if (m_w_jpg) {
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("ap_useJpgProxy"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::HorizontalSlider("ap_jpgQuality"));
        mainGrid->setChild(row, 1, new tinia::model::gui::Label("ap_jpgQuality", false));
        mainGrid->setChild(row, 2, new tinia::model::gui::Label("ap_jpgQuality", true));
        row++;
    }

    if (m_w_ap) {
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("ap_useAutoProxy"));
        row++;
    }

    if (m_w_ap && m_w_apd) {
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("ap_autoProxyDebugging"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::Label("ap_autoProxyAlgo"));
        mainGrid->setChild(row, 1, new tinia::model::gui::ComboBox("ap_autoProxyAlgo"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("ap_debugSplatCol"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("ap_decayMode"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("ap_roundSplats"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("ap_screenSpaceSized"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::HorizontalSlider("ap_overlap"));
        mainGrid->setChild(row, 1, new tinia::model::gui::Label("ap_overlap", false));
        mainGrid->setChild(row, 2, new tinia::model::gui::Label("ap_overlap", true));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("ap_alwaysShowMostRecent"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::HorizontalSlider("ap_splats"));
        mainGrid->setChild(row, 1, new tinia::model::gui::Label("ap_splats", false));
        mainGrid->setChild(row, 2, new tinia::model::gui::Label("ap_splats", true));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::Button("ap_resetAllModels"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("ap_useISTC"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("ap_splatOutline"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::Button("ap_reloadShader"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("ap_useFragExt"));
        mainGrid->setChild(row, 1, new tinia::model::gui::Label("ap_fragExtStatus", true)); // true) We get the text string connected to the element, false) name of element
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::Label("ap_consoleLog", false));
        mainGrid->setChild(row, 1, new tinia::model::gui::Label("ap_consoleLog", true));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::VerticalExpandingSpace());
    }

    if (m_with_depth_buffer_manipulation) {
        mainGrid->setChild(row, 0, new tinia::model::gui::HorizontalSlider("ap_depthWidth"));
        mainGrid->setChild(row, 1, new tinia::model::gui::Label("ap_depthWidth", false));
        mainGrid->setChild(row, 2, new tinia::model::gui::Label("ap_depthWidth", true));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::HorizontalSlider("ap_depthHeight"));
        mainGrid->setChild(row, 1, new tinia::model::gui::Label("ap_depthHeight", false));
        mainGrid->setChild(row, 2, new tinia::model::gui::Label("ap_depthHeight", true));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::Button("ap_set_depth_size_32"));
        mainGrid->setChild(row, 1, new tinia::model::gui::Button("ap_set_depth_size_64"));
        mainGrid->setChild(row, 2, new tinia::model::gui::Button("ap_set_depth_size_128"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::Button("ap_set_depth_size_256"));
        mainGrid->setChild(row, 1, new tinia::model::gui::Button("ap_set_depth_size_512"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("ap_mid_texel_sampling"));
        mainGrid->setChild(row, 1, new tinia::model::gui::CheckBox("ap_small_delta_sampling"));
        mainGrid->setChild(row, 2, new tinia::model::gui::CheckBox("ap_larger_delta_sampling"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("ap_bi_linear_filtering"));
        mainGrid->setChild(row, 1, new tinia::model::gui::CheckBox("ap_use_qt_img_scaling"));
        mainGrid->setChild(row, 2, new tinia::model::gui::CheckBox("ap_mid_splat_sampling"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("ap_simulate_downsampling"));
        mainGrid->setChild(row, 1, new tinia::model::gui::CheckBox("ap_16_bit_depth"));
        mainGrid->setChild(row, 2, new tinia::model::gui::CheckBox("ap_hold_up_png"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::Button("ap_whitebg"));
        mainGrid->setChild(row, 1, new tinia::model::gui::Button("ap_greenbg"));
        mainGrid->setChild(row, 2, new tinia::model::gui::Button("ap_blackbg"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("ap_dump"));
        mainGrid->setChild(row, 1, new tinia::model::gui::Button("ap_dump_button"));
        row++;
    }

    return mainGrid;
}




} // of namespace utils
} // of namespace tinia
