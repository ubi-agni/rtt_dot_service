/******************************************************************************
*                           OROCOS dot service                                *
*                                                                             *
*                         (C) 2011 Steven Bellens                             *
*                     steven.bellens@mech.kuleuven.be                         *
*                    Department of Mechanical Engineering,                    *
*                   Katholieke Universiteit Leuven, Belgium.                  *
*                                                                             *
*       You may redistribute this software and/or modify it under either the  *
*       terms of the GNU Lesser General Public License version 2.1 (LGPLv2.1  *
*       <http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html>) or (at your *
*       discretion) of the Modified BSD License:                              *
*       Redistribution and use in source and binary forms, with or without    *
*       modification, are permitted provided that the following conditions    *
*       are met:                                                              *
*       1. Redistributions of source code must retain the above copyright     *
*       notice, this list of conditions and the following disclaimer.         *
*       2. Redistributions in binary form must reproduce the above copyright  *
*       notice, this list of conditions and the following disclaimer in the   *
*       documentation and/or other materials provided with the distribution.  *
*       3. The name of the author may not be used to endorse or promote       *
*       products derived from this software without specific prior written    *
*       permission.                                                           *
*       THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR  *
*       IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED        *
*       WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE    *
*       ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,*
*       INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES    *
*       (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS       *
*       OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) *
*       HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,   *
*       STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING *
*       IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE    *
*       POSSIBILITY OF SUCH DAMAGE.                                           *
*                                                                             *
*******************************************************************************/

#include "rtt_dot_service.hpp"
#include <fstream>
#include <rtt/rtt-config.h>
#include <sstream>

using namespace RTT;

Dot::Dot(TaskContext* owner)
    : Service("dot", owner), base::ExecutableInterface()
    ,m_dot_file("orograph.dot")
    ,m_comp_args("style=\"rounded,filled\",fontsize=15,color=\"#777777\",fillcolor=\"#eeeeee\",")
    ,m_conn_args(" ")
    ,m_chan_args("shape=record,")
{
    this->addOperation("getOwnerName", &Dot::getOwnerName, this).doc("Returns the name of the owner of this object.");
    this->addOperation("generate", &Dot::execute, this).doc("Generate component overview and write to 'dot_file'.");
    this->addProperty("dot_file", m_dot_file).doc("File to write the generated dot syntax to.");
    this->addProperty("comp_args", m_comp_args).doc("Arguments to add to the component drawings.");
    this->addProperty("conn_args", m_conn_args).doc("Arguments to add to the connection drawings.");
    this->addProperty("chan_args", m_chan_args).doc("Arguments to add to the channel drawings.");
    this->doc("Dot service interface.");
    //owner->engine()->runFunction(this);
}

std::string Dot::getOwnerName()
{
    return getOwner()->getName();
}

std::string Dot::quote(std::string const& name)
{
  return "\"" + name + "\"";
}

void Dot::scanService(std::string path, Service::shared_ptr sv)
{
    std::vector<std::string> comp_ports;
    comp_ports.clear();
    // Get all component ports
    comp_ports = sv->getPortNames();

    // Loop over all ports
    for(unsigned int j = 0; j < comp_ports.size(); j++){
      log(Debug) << "        Port: " << comp_ports[j] << endlog();
#if RTT_VERSION_GTE(2,8,99)
      std::list<internal::ConnectionManager::ChannelDescriptor> chns = sv->getPort(comp_ports[j])->getManager()->getConnections();
#else
      std::list<internal::ConnectionManager::ChannelDescriptor> chns = sv->getPort(comp_ports[j])->getManager()->getChannels();
#endif
      std::list<internal::ConnectionManager::ChannelDescriptor>::iterator k;

      bool is_input_port = (dynamic_cast<base::InputPortInterface*>(sv->getPort(comp_ports[j])) != 0);

      for(k = chns.begin(); k != chns.end(); k++){
        base::ChannelElementBase::shared_ptr bs = k->get<1>();
        ConnPolicy cp = k->get<2>();

        // Get input component name
        std::string comp_in, port_in;
        if(bs->getInputEndPoint()->getPort() != 0){
          if (bs->getInputEndPoint()->getPort()->getInterface() != 0 ){
            comp_in = bs->getInputEndPoint()->getPort()->getInterface()->getOwner()->getName();
          }
          else{
            comp_in = "free input ports";
          }
          port_in = bs->getInputEndPoint()->getPort()->getName();
        }

        // Get output component name
        std::string comp_out, port_out;
        if(bs->getOutputEndPoint()->getPort() != 0){
          if (bs->getOutputEndPoint()->getPort()->getInterface() != 0 ){
            comp_out = bs->getOutputEndPoint()->getPort()->getInterface()->getOwner()->getName();
          }
          else{
            comp_out = "free output ports";
          }
          port_out = bs->getOutputEndPoint()->getPort()->getName();
        }

        log(Debug) << "          Connection : [" << comp_in << "] "<<port_in<<" <-> "<< "[" << comp_out << "] " << port_out << endlog();


        std::string conn_info;
        std::stringstream ss;
        switch(cp.type){
        case ConnPolicy::DATA:
            conn_info = "data";
            break;
        case ConnPolicy::BUFFER:
            ss << "buffer [ " << cp.size << " ]";
            conn_info = ss.str();
            break;
        case ConnPolicy::CIRCULAR_BUFFER:
            ss << "circbuffer [ " << cp.size << " ]";
            conn_info = ss.str();
            break;
        default:
            conn_info = "unknown";
        }
        log(Debug) << "      Connection has conn_info: " << conn_info << endlog();
        // Only consider input ports
        if(is_input_port){
          // First, consider regular connections
          if(!comp_in.empty()){
            // If the ConnPolicy has a non-empty name, use that name as the topic name
            if(!cp.name_id.empty()){
              // plot the channel element as a seperate box and connect input and output with it
              m_dot << comp_in <<":"<<comp_ports_map[comp_in][port_in] << " -> " << quote(cp.name_id) << " [color=\"#2a4563\",style=bold];\n";
            //   m_dot << quote(cp.name_id) << "[" << m_chan_args << "label=" << quote(cp.name_id) << "];\n";
            //   m_dot << quote(comp_in) << "->" << quote(cp.name_id) << "[" << m_conn_args << "label=" << quote(port_in) << "];\n";
            //   m_dot << quote(cp.name_id) << "->" << quote(comp_out) << "[" << m_conn_args << "label=" << quote(port_out) << "];\n";
            }
            // Else, use a custom name: compInportIncompOutportOut
            else{
              // plot the channel element as a seperate box and connect input and output with it
              m_dot << comp_in <<":"<<comp_ports_map[comp_in][port_in] <<" -> "<< comp_out <<":"<<comp_ports_map[comp_out][port_out]<< " [color=\"#2a4563\",style=bold];\n";
            //   m_dot << quote(comp_in + port_in + comp_out + port_out) << "[" << m_chan_args << "label=" << quote(conn_info) << "];\n";
            //   m_dot << quote(comp_in) << "->" << quote(comp_in + port_in + comp_out + port_out) << "[" << m_conn_args << "label=" << quote(port_in) << "];\n";
            //   m_dot << quote(comp_in + port_in + comp_out + port_out) << "->" << comp_out << "[" << m_conn_args << "label=" << quote(port_out) << "];\n";
            }
          }else{
              m_dot << quote(cp.name_id) <<" -> "<< comp_out <<":"<<comp_ports_map[comp_out][port_out];
              switch(cp.transport)
              {
                  case 1:
                    m_dot << " [" << m_conn_args << "label=CORBA,style=dashed];";
                    break;
                  case 2:
                    m_dot << " [" << m_conn_args << "label=MQ,style=dashed];";
                    break;
                  case 3:
                    m_dot << " [" << m_conn_args << "label=ROS,style=dashed];";
                    break;
                default:
                    break;
              }
              m_dot << "\n";
            // m_dot << quote(comp_out + port_out) << "[" << m_chan_args << "label=" << quote(conn_info) << "];\n";
            // m_dot << quote(comp_out + port_out) << "->" << quote(comp_out) << "[" << m_conn_args << "label=" << quote(port_out) << "];\n";
          }
        }
        else{
          // Consider only output ports that do not have a corresponding input port
          if(comp_out.empty()){
            // If the ConnPolicy has a non-empty name, use that name as the topic name
            if(!cp.name_id.empty())
            {
              // plot the channel element as a seperate box and connect input and output with it
              m_dot << comp_in <<":"<<comp_ports_map[comp_in][port_in] << " -> " << quote(cp.name_id);
              switch(cp.transport)
              {
                  case 1:
                    m_dot << " [" << m_conn_args << "label=CORBA,style=dashed];";
                    break;
                  case 2:
                    m_dot << " [" << m_conn_args << "label=MQ,style=dashed];";
                    break;
                  case 3:
                    m_dot << " [" << m_conn_args << "label=ROS,style=dashed];";
                    break;
                default:
                    break;
              }
              m_dot << "\n";
            }
            else
            {
              // plot the channel element as a seperate box and connect input and output with it
              //log(Debug) << "case 22" << std::endlog();
              //m_dot << comp_in <<":"<<comp_ports_map[comp_in][port_in] << " -> " << quote(cp.name_id);
              //m_dot << comp_in <<":"<<comp_ports_map[mpeer][point_in] << "[label=" << conn_info << "];\n";
             // m_dot << comp_in << "->" <<  comp_in + port_in) << "[" << m_conn_args << "label=" << port_in) << "];\n";
            }
          }
          else {
              log(Debug) << "        Dropped " << conn_info << " from " << comp_out + port_out << " to " << comp_in + port_in << endlog();
          }
        }
      }
    }
    // Recurse for sub services
    Service::ProviderNames providers = sv->getProviderNames();
    for(Service::ProviderNames::iterator it=providers.begin(); it != providers.end(); ++it) {
          scanService(path + "." + sv->getName(), sv->provides(*it));
    }
}

std::string Dot::appendToPath(const std::string& path,const std::string& sub)
{
    if(path.empty())
        return sub;
    return path + "." + sub;
}

void Dot::buildComponentOutputPortsMap(std::string path, Service::shared_ptr sv, int& current_count)
{
    std::vector<std::string> comp_ports = sv->getPortNames();
    // Build the output ports
    for(unsigned int j = 0; j < comp_ports.size(); j++)
    {
    bool is_output_port = (dynamic_cast<base::OutputPortInterface*>(sv->getPort(comp_ports[j])) != 0);
    if(is_output_port)
    {
        comp_ports_map[mpeer][comp_ports[j]] = "o" + std::to_string(current_count);
        m_dot << (current_count>0 ? " | ":"") << "<o" << std::to_string(current_count) <<">"<< comp_ports[j];
        current_count++;
    }
    }
    // recurse for ouputs
    Service::ProviderNames providers = sv->getProviderNames();
    for(Service::ProviderNames::iterator it=providers.begin(); it != providers.end(); ++it) {
        buildComponentOutputPortsMap(appendToPath(path , sv->getName()), sv->provides(*it) ,current_count);
    }
}

void Dot::buildComponentInputPortsMap(std::string path, Service::shared_ptr sv, int& current_count)
{
    log(Debug) << "Dot::buildComponentInputPortsMap path=" << path << " sv=" << sv->getName() << " current_count=" << current_count << endlog();

    // Build the input ports
    for(auto port : sv->getPortNames())
    {
        std::cout << "     port " << port << endlog();
        bool is_input_port = (dynamic_cast<base::InputPortInterface*>(sv->getPort(port)) != 0);
        if(is_input_port)
        {
            log(Debug) << "         ---> is input" << endlog();
            comp_ports_map[mpeer][port] = "i" + std::to_string(current_count);
            m_dot << (current_count>0 ? " | ":"") << "<i" << std::to_string(current_count) <<">"<< port;
            current_count++;
        }
    }
//     for(auto operation_name : sv->getOperationNames())
//     {
//         std::cout << "     operation " << operation_name << endlog();
//         auto operation = sv->getOperation(operation_name);
//         log(Debug) << "         ---> is input" << endlog();
//         comp_ports_map[mpeer][operation_name] = "i" + std::to_string(current_count);
//         m_dot << (current_count>0 ? " | ":"") << "<i" << std::to_string(current_count) <<">"<< operation_name;
//         current_count++;
//     }
    // recurse for inputs
    Service::ProviderNames providers = sv->getProviderNames();
    for(Service::ProviderNames::iterator it=providers.begin(); it != providers.end(); ++it)
    {
        buildComponentInputPortsMap(appendToPath(path , sv->getName()), sv->provides(*it),current_count);
    }
}


bool Dot::execute()
{
  m_dot.str("");
  m_dot << "digraph G { \n";
  m_dot << "graph[splines=true, overlap=false] \n";
  m_dot << "rankdir=LR; \n";
  m_dot << "nodesep=0.5; \n";
  m_dot << "ranksep=1.5; \n";
  m_dot << "fontname=\"sans\";\n";
  // m_dot << "labelloc=\"t\";\n";
  // m_dot << "fontsize=25;\n";
  // m_dot << "label=" << this->getOwner()->getName()<<";\n";
  m_dot << "node [style=\"rounded,filled\",fontsize=15,color=\"#777777\",fillcolor=\"#eeeeee\"];\n";
  // List all peers of this component
  std::vector<std::string> peerList = this->getOwner()->getPeerList();
  // Add the component itself as well
  if(peerList.size() == 0)
  {
    log(Debug) << "Component has no peers!" << endlog();
    return false;
  }

  // Reset the map
  comp_ports_map.clear();

  for(unsigned int i = 0; i < peerList.size(); i++)
  {
    mpeer = peerList[i];
    log(Debug) <<"["<<i<<"] Component: " << mpeer << endlog();
    // Get a pointer to the taskcontext, which can be either a peer or the component itself.
    TaskContext* tc = this->getOwner()->getPeer(mpeer);
    if(tc == 0)
    {
      tc = this->getOwner();
    }

    base::TaskCore::TaskState st = tc->getTaskState();

    std::string st_str,color;
    switch (st)
    {
        case base::TaskCore::Init          : st_str = "Init          ";color = "white";       break;
        case base::TaskCore::PreOperational: st_str = "PreOperational";color = "orange";      break;
        case base::TaskCore::FatalError    : st_str = "FatalError    ";color = "red";         break;
        case base::TaskCore::Exception     : st_str = "Exception     ";color = "red";         break;
        case base::TaskCore::Stopped       : st_str = "Stopped       ";color = "lightblue";   break;
        case base::TaskCore::Running       : st_str = "Running       ";color = "#4ec167";     break;
        case base::TaskCore::RunTimeError  : st_str = "RunTimeError  ";color = "red";         break;
    }

    m_dot << quote(mpeer) << "[shape=record,fillcolor=\""<<color<<"\",label=\"\\N|{{";
    int current_count = 0;
    buildComponentInputPortsMap("",tc->provides(),current_count);
    m_dot << " } | | { ";
    current_count = 0;
    buildComponentOutputPortsMap("",tc->provides(),current_count);
    m_dot << "}}\"];\n";
  }
  // Loop over all peers + own component
  for(unsigned int i = 0; i < peerList.size(); i++)
  {
    mpeer = peerList[i];
    log(Debug) <<"["<<i<<"] Component: " << mpeer << endlog();
    // Get a pointer to the taskcontext, which can be either a peer or the component itself.
    TaskContext* tc = this->getOwner()->getPeer(mpeer);
    if(this->getOwner()->getPeer(mpeer) == 0)
    {
      tc = this->getOwner();
    }

    // Draw in/out ports
    scanService("",tc->provides());
  }
  m_dot << "}\n";
  std::ofstream fl(m_dot_file.c_str());
  if (fl.is_open())
  {
    fl << m_dot.str();
    fl.close();
    return true;
  }
  else
  {
    log(Debug) << "Unable to open file: " << m_dot_file << endlog();
    return false;
  }
}
ORO_SERVICE_NAMED_PLUGIN(Dot, "dot")
